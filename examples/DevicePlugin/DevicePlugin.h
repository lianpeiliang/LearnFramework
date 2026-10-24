#pragma once

#include "Plugin.h"
#include "WidgetProvider.h"
#include "PluginServices.h"
#include <QObject>

// 两个面板共享插件内的设备状态，仅演示模拟设备，不连接真实硬件。
class DeviceState : public QObject
{
    Q_OBJECT
public:
    QString name = QStringLiteral("示例设备");
    int refreshInterval = 1000;
    bool connected = false;

signals:
    void changed();
};

class DevicePlugin : public QObject, public Plugin, public WidgetProvider, public PluginServicesAware
{
    Q_OBJECT
    Q_PLUGIN_METADATA(IID "org.learnframework.Plugin/1.0")
    Q_INTERFACES(Plugin WidgetProvider PluginServicesAware)
public:
    bool initialize() override;
    bool start() override { return true; }
    bool stop() override { return true; }
    bool shutdown() override;
    QStringList widgetClasses() const override;
    QWidget *createWidget(const QString &name, QWidget *parent) override;
    void setPluginServices(PluginServices *value) override { services = value; }

private:
    DeviceState *state = nullptr;
    PluginServices *services = nullptr;
    void publishState();
};
