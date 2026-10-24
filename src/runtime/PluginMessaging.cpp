#include "PluginMessaging.h"

#include <QPointer>
#include <QThread>

namespace
{
// 将一个插件回调适配为旧 CommandBus 的处理器；注册成功后由总线管理引用。
class PluginCommand : public CommandHandler
{
public:
    PluginCommand(QObject *owner, PluginServices::Command command)
        : owner(owner), command(std::move(command)) {}

    bool dispatchCommand(unsigned int, char *data, int length) override
    {
        // data 指向 sendCommand 栈上的 QVariantMap，length 是对象大小，非序列化长度。
        if (!owner || !data || length != static_cast<int>(sizeof(QVariantMap)))
            return false;
        // 回调可能销毁 owner 并注销当前处理器，调用期间持有回调副本。
        const auto callback = command;
        return callback(*reinterpret_cast<const QVariantMap *>(data));
    }
    bool dispatchMessage(unsigned int, void *, int) override { return false; }

private:
    // owner 销毁后自动置空，防止继续调用依赖该对象的插件回调。
    QPointer<QObject> owner;
    PluginServices::Command command;
};
}

PluginMessageHub::PluginMessageHub(CommandBus *bus) : bus(bus) {}

PluginMessageHub::~PluginMessageHub()
{
    // CommandBus 的消息订阅不持有对象，必须在中转对象销毁前主动退订。
    for (unsigned int id : observed)
        bus->unsubscribe(id, this);
}

void PluginMessageHub::observe(unsigned int id)
{
    if (!observed.contains(id))
    {
        bus->subscribe(id, this);
        observed.insert(id);
    }
}

bool PluginMessageHub::dispatchMessage(unsigned int id, void *data, int length)
{
    if (!data || length != static_cast<int>(sizeof(QVariantMap)))
        return false;
    // 同一个消息只在 CommandBus 上订阅一次，插件订阅者交给 Qt 管理。
    // 订阅者可在回调中销毁其他窗口，Qt 会跳过已经失效的连接。
    emit messagePublished(id, *static_cast<const QVariantMap *>(data));
    return true;
}

PluginServicesScope::PluginServicesScope(std::shared_ptr<PluginMessageHub> hub)
    : hub(std::move(hub)) {}

PluginServicesScope::~PluginServicesScope() { clear(); }

bool PluginServicesScope::onGuiThread() const
{
    return QThread::currentThread() == hub->thread();
}

bool PluginServicesScope::registerCommand(unsigned int id, QObject *owner, Command command)
{
    // 预留较小编号给框架核心，同时拒绝跨线程注册或没有生命周期归属的回调。
    if (!onGuiThread() || id < 2000 || !owner || owner->thread() != hub->thread() || !command)
        return false;
    auto *handler = new PluginCommand(owner, std::move(command));
    if (hub->commandBus()->registerHandler(id, handler) != 0)
    {
        // 编号冲突时总线没有接管 handler，释放本次申请且保留原有处理器。
        delete handler;
        return false;
    }
    // 以作用域作为连接上下文；owner 或作用域结束后，均不会留下有效的注册回调。
    const auto connection = connect(owner, &QObject::destroyed, this, [this, id] { removeCommand(id); });
    commands.insert(id, {handler, connection});
    return true;
}

void PluginServicesScope::removeCommand(unsigned int id)
{
    auto it = commands.find(id);
    if (it == commands.end())
        return;
    const Registration registration = it.value();
    // 先移除本地记录并断开销毁通知，避免清理过程再次处理同一注册项。
    commands.erase(it);
    disconnect(registration.ownerDestroyed);
    // 命令可能已被外部替换，只注销仍然属于本作用域的那个处理器。
    if (hub->commandBus()->handlerFor(id) == registration.handler)
        hub->commandBus()->unregisterHandler(id);
}

bool PluginServicesScope::sendCommand(unsigned int id, const QVariantMap &data)
{
    if (!onGuiThread() || id < 2000)
        return false;
    // 为旧接口提供临时可写地址；同步分发返回后 payload 销毁，接收方不能保存其地址。
    // 这是同进程对象指针传递，不是可跨线程或跨进程使用的二进制协议。
    QVariantMap payload = data;
    return hub->commandBus()->dispatchCommand(id, reinterpret_cast<char *>(&payload), sizeof(payload)) != 0;
}

bool PluginServicesScope::subscribe(unsigned int id, QObject *owner, Message callback)
{
    if (!onGuiThread() || id < 2000 || !owner || owner->thread() != hub->thread() || !callback)
        return false;
    hub->observe(id);
    // Qt 已自动断开已销毁 owner 的连接，这里只清理作用域中的无效记录。
    for (int i = subscriptions.size() - 1; i >= 0; --i)
        if (subscriptions.at(i).owner.isNull())
            subscriptions.removeAt(i);
    // owner 是接收上下文：窗口销毁时 Qt 自动断开连接，并释放捕获的 callback。
    const auto connection = connect(hub.get(), &PluginMessageHub::messagePublished, owner,
        [id, callback](unsigned int publishedId, const QVariantMap &data) {
            if (publishedId == id)
                callback(data);
        });
    subscriptions.append({owner, connection});
    return true;
}

bool PluginServicesScope::publish(unsigned int id, const QVariantMap &data)
{
    if (!onGuiThread() || id < 2000)
        return false;
    // 与命令相同，消息载荷仅在本次同步调用期间有效；需要保存时由接收方复制。
    QVariantMap payload = data;
    hub->commandBus()->dispatchMessage(id, &payload, sizeof(payload));
    return true;
}

void PluginServicesScope::clear()
{
    // 先断开消息回调，再撤销命令；即使 owner 仍然存在也能清空插件的通信入口。
    for (const auto &subscription : subscriptions)
        disconnect(subscription.connection);
    subscriptions.clear();
    // removeCommand 会修改 commands，遍历键的副本以免迭代器失效。
    const auto ids = commands.keys();
    for (unsigned int id : ids)
        removeCommand(id);
}
