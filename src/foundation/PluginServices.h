#pragma once

#include <QVariantMap>
#include <QtPlugin>
#include <functional>

class QObject;

// 主程序提供实现；插件只持有借用接口，不链接主程序实现。
// 同步 GUI 线程调用。插件命令/消息编号使用 2000 及以上，避免占用核心命令。
class PluginServices
{
public:
    using Command = std::function<bool(const QVariantMap &)>;
    using Message = std::function<void(const QVariantMap &)>;
    virtual ~PluginServices() = default;
    virtual bool registerCommand(unsigned int id, QObject *owner, Command command) = 0;
    virtual bool sendCommand(unsigned int id, const QVariantMap &data = {}) = 0;
    virtual bool subscribe(unsigned int id, QObject *owner, Message callback) = 0;
    virtual bool publish(unsigned int id, const QVariantMap &data) = 0;
};

// 可选能力，不修改原有 Plugin/1.0 或 WidgetProvider/1.0 的虚函数表。
class PluginServicesAware
{
public:
    virtual ~PluginServicesAware() = default;
    // initialize 前注入，shutdown 后撤回；不能 delete 或保存到插件生命周期之外。
    virtual void setPluginServices(PluginServices *services) = 0;
};

Q_DECLARE_INTERFACE(PluginServicesAware, "org.learnframework.PluginServicesAware/1.0")
