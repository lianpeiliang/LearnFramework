#include "PluginMessaging.h"

#include <QPointer>
#include <QThread>

namespace
{
class PluginCommand : public CommandHandler
{
public:
    PluginCommand(QObject *owner, PluginServices::Command command)
        : owner(owner), command(std::move(command)) {}

    bool dispatchCommand(unsigned int, char *data, int length) override
    {
        if (!owner || !data || length != static_cast<int>(sizeof(QVariantMap)))
            return false;
        // 回调可能销毁 owner 并注销当前处理器，调用期间持有回调副本。
        const auto callback = command;
        return callback(*reinterpret_cast<const QVariantMap *>(data));
    }
    bool dispatchMessage(unsigned int, void *, int) override { return false; }

private:
    QPointer<QObject> owner;
    PluginServices::Command command;
};
}

PluginMessageHub::PluginMessageHub(CommandBus *bus) : bus(bus) {}

PluginMessageHub::~PluginMessageHub()
{
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
    if (!onGuiThread() || id < 2000 || !owner || owner->thread() != hub->thread() || !command)
        return false;
    auto *handler = new PluginCommand(owner, std::move(command));
    if (hub->commandBus()->registerHandler(id, handler) != 0)
    {
        delete handler;
        return false;
    }
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
    commands.erase(it);
    disconnect(registration.ownerDestroyed);
    if (hub->commandBus()->handlerFor(id) == registration.handler)
        hub->commandBus()->unregisterHandler(id);
}

bool PluginServicesScope::sendCommand(unsigned int id, const QVariantMap &data)
{
    if (!onGuiThread() || id < 2000)
        return false;
    QVariantMap payload = data;
    return hub->commandBus()->dispatchCommand(id, reinterpret_cast<char *>(&payload), sizeof(payload)) != 0;
}

bool PluginServicesScope::subscribe(unsigned int id, QObject *owner, Message callback)
{
    if (!onGuiThread() || id < 2000 || !owner || owner->thread() != hub->thread() || !callback)
        return false;
    hub->observe(id);
    for (int i = subscriptions.size() - 1; i >= 0; --i)
        if (subscriptions.at(i).owner.isNull())
            subscriptions.removeAt(i);
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
    QVariantMap payload = data;
    hub->commandBus()->dispatchMessage(id, &payload, sizeof(payload));
    return true;
}

void PluginServicesScope::clear()
{
    for (const auto &subscription : subscriptions)
        disconnect(subscription.connection);
    subscriptions.clear();
    const auto ids = commands.keys();
    for (unsigned int id : ids)
        removeCommand(id);
}
