#pragma once

#include "Plugin.h"
#include "WidgetProvider.h"
#include "PluginServices.h"
#include <QObject>

class DeviceState;

// WidgetProvider 提供监控/配置控件；PluginServicesAware 接收主程序注入的通信服务。
// 其他插件只需遵守 DeviceProtocol，无须链接本插件的业务类。
class DevicePlugin : public QObject, public Plugin, public WidgetProvider, public PluginServicesAware
{
    Q_OBJECT
    // 元数据声明 Qt 插件入口，接口列表声明主程序可查询到的必需/可选能力。
    Q_PLUGIN_METADATA(IID "org.learnframework.Plugin/1.0")
    Q_INTERFACES(Plugin WidgetProvider PluginServicesAware)
public:
    bool initialize() override;
    // 模拟设备不启动线程或定时器，refreshInterval 仅用于展示配置。
    bool start() override { return true; }
    bool stop() override { return true; }
    bool shutdown() override;
    QStringList widgetClasses() const override;
    QWidget *createWidget(const QString &name, QWidget *parent) override;
    void setPluginServices(PluginServices *value) override { services = value; }

private:
    // 状态由插件拥有；所有面板借用它，关闭时先由加载器销毁面板。
    DeviceState *state = nullptr;
    // 借用主程序的服务作用域，不由插件 delete；没有服务时仍可使用本地面板。
    PluginServices *services = nullptr;
    bool registerCommands();
    bool handleSetConnection(const QVariantMap &data);
    bool handleQueryStatus();
    void publishState();
};
