#pragma once

#include "Plugin.h"
#include "WidgetProvider.h"
#include "PluginServices.h"
#include <QObject>

// 提供 F/G 两种内容控件，并通过注入的主程序服务与设备插件通信。
// WidgetProvider 和 PluginServicesAware 是独立于 Plugin 生命周期的可选能力。
class ContentPlugin : public QObject, public Plugin, public WidgetProvider, public PluginServicesAware
{
    Q_OBJECT
    // Qt 通过元数据加载入口，再通过接口声明查询控件工厂和通信服务能力。
    Q_PLUGIN_METADATA(IID "org.learnframework.Plugin/1.0")
    Q_INTERFACES(Plugin WidgetProvider PluginServicesAware)

public:
    bool initialize() override;
    // 此插件没有后台任务，控件在工厂请求时才创建，无须额外启动或停止。
    bool start() override { return true; }
    bool stop() override { return true; }
    bool shutdown() override;
    QStringList widgetClasses() const override;
    QWidget *createWidget(const QString &name, QWidget *parent) override;
    void setPluginServices(PluginServices *value) override { services = value; }

private:
    bool initialized = false;
    // 主程序拥有服务；插件和创建出的控件仅借用，允许为空以兼容无通信的宿主。
    PluginServices *services = nullptr;
};
