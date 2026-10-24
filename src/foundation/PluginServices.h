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
    // 载荷采用 QVariantMap 约定字段，回调接收的引用仅在本次同步调用期间有效。
    using Command = std::function<bool(const QVariantMap &)>;
    using Message = std::function<void(const QVariantMap &)>;
    virtual ~PluginServices() = default;
    // 同一时刻每个命令 ID 只能绑定一个回调；owner 销毁或服务作用域清理时自动解除注册。
    // owner 必须与主程序消息中转对象处于同一 GUI 线程，空回调和无效参数返回 false。
    virtual bool registerCommand(unsigned int id, QObject *owner, Command command) = 0;
    // 同步执行并返回命令回调结果；无对应命令、线程不符或编号低于 2000 也返回 false。
    virtual bool sendCommand(unsigned int id, const QVariantMap &data = {}) = 0;
    // 一对多订阅；owner 销毁后不再调用回调，多次订阅会形成多份通知连接。
    virtual bool subscribe(unsigned int id, QObject *owner, Message callback) = 0;
    // true 表示通过线程与编号检查并完成发布，不保证存在订阅者或业务处理成功。
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
