#pragma once

#include "CommandBus.h"
#include "PluginServices.h"

#include <QHash>
#include <QMetaObject>
#include <QObject>
#include <QPointer>
#include <QSet>
#include <memory>

// 每个加载器共享一个中转对象，在已有 CommandBus 与 Qt 的生命周期管理间适配。
class PluginMessageHub : public QObject, public CommandHandler
{
    Q_OBJECT
public:
    explicit PluginMessageHub(CommandBus *bus);
    ~PluginMessageHub() override;
    CommandBus *commandBus() const { return bus; }
    void observe(unsigned int id);
    bool dispatchCommand(unsigned int, char *, int) override { return false; }
    bool dispatchMessage(unsigned int id, void *data, int length) override;

signals:
    void messagePublished(unsigned int id, const QVariantMap &data);

private:
    CommandBus *bus;
    QSet<unsigned int> observed;
};

// 每个插件有独立作用域。窗口/插件销毁时自动移除回调，DLL 卸载前统一清理。
class PluginServicesScope : public QObject, public PluginServices
{
public:
    explicit PluginServicesScope(std::shared_ptr<PluginMessageHub> hub);
    ~PluginServicesScope() override;
    bool registerCommand(unsigned int id, QObject *owner, Command command) override;
    bool sendCommand(unsigned int id, const QVariantMap &data = {}) override;
    bool subscribe(unsigned int id, QObject *owner, Message callback) override;
    bool publish(unsigned int id, const QVariantMap &data) override;
    void clear();

private:
    struct Registration
    {
        CommandHandler *handler;
        QMetaObject::Connection ownerDestroyed;
    };
    struct Subscription
    {
        QPointer<QObject> owner;
        QMetaObject::Connection connection;
    };
    bool onGuiThread() const;
    void removeCommand(unsigned int id);
    std::shared_ptr<PluginMessageHub> hub;
    QHash<unsigned int, Registration> commands;
    QList<Subscription> subscriptions;
};
