#pragma once

#include "Plugin.h"
#include "WidgetProvider.h"
#include "PluginServices.h"
#include <QObject>

class ContentPlugin : public QObject, public Plugin, public WidgetProvider, public PluginServicesAware
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
    bool initialized = false;
    PluginServices *services = nullptr;
};
