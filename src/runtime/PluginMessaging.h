#pragma once

#include "CommandBus.h"
#include "PluginServices.h"

#include <QHash>
#include <QMetaObject>
#include <QObject>
#include <QPointer>
#include <QSet>
#include <memory>

// 同一加载器下的插件共享此中转对象，将 CommandBus 消息转换为 Qt 信号。
// 总线只记录中转对象，具体插件/窗口的订阅者由 Qt 连接及其上下文对象管理。
class PluginMessageHub : public QObject, public CommandHandler
{
    Q_OBJECT
public:
    explicit PluginMessageHub(CommandBus *bus);
    ~PluginMessageHub() override;
    CommandBus *commandBus() const { return bus; }
    // 对同一消息编号只向总线订阅一次，避免每个窗口重复转发整条消息。
    void observe(unsigned int id);
    bool dispatchCommand(unsigned int, char *, int) override { return false; }
    bool dispatchMessage(unsigned int id, void *data, int length) override;

signals:
    // 所有编号共用一个信号，由各订阅连接筛选自己关注的编号。
    void messagePublished(unsigned int id, const QVariantMap &data);

private:
    // 借用主程序的总线，须比中转对象活得更久；observed 用于析构时退订。
    CommandBus *bus;
    QSet<unsigned int> observed;
};

// 每个插件有独立作用域。窗口/插件销毁时自动移除回调，DLL 卸载前统一清理。
class PluginServicesScope : public QObject, public PluginServices
{
public:
    explicit PluginServicesScope(std::shared_ptr<PluginMessageHub> hub);
    ~PluginServicesScope() override;
    // 以下通信接口同步执行，限定在 hub 所在线程（应用中为 GUI 线程）。
    // 插件只使用 2000 及以上编号；注册的 owner 也须位于同一线程。
    bool registerCommand(unsigned int id, QObject *owner, Command command) override;
    bool sendCommand(unsigned int id, const QVariantMap &data = {}) override;
    bool subscribe(unsigned int id, QObject *owner, Message callback) override;
    // true 表示调用参数通过检查并完成发布，不代表一定有订阅者接收。
    bool publish(unsigned int id, const QVariantMap &data) override;
    // 断开本作用域的订阅并注销命令，销毁 DLL 之前释放其中的回调对象。
    void clear();

private:
    struct Registration
    {
        // 处理器由 CommandBus 的注册引用管理；这里仅用于核对注销目标。
        CommandHandler *handler;
        QMetaObject::Connection ownerDestroyed;
    };
    struct Subscription
    {
        // 弱引用仅检查窗口是否还存在，不延长窗口生命周期。
        QPointer<QObject> owner;
        QMetaObject::Connection connection;
    };
    bool onGuiThread() const;
    void removeCommand(unsigned int id);
    std::shared_ptr<PluginMessageHub> hub;
    QHash<unsigned int, Registration> commands;
    QList<Subscription> subscriptions;
};
