#pragma once

#include "Plugin.h"
#include "WidgetProvider.h"
#include <QObject>

class ContentPlugin : public QObject, public Plugin, public WidgetProvider
{
    Q_OBJECT
    Q_PLUGIN_METADATA(IID "org.learnframework.Plugin/1.0")
    Q_INTERFACES(Plugin WidgetProvider)

public:
    bool initialize() override;
    bool start() override { return true; }
    bool stop() override { return true; }
    bool shutdown() override;
    QStringList widgetClasses() const override;
    QWidget *createWidget(const QString &name, QWidget *parent) override;

private:
    bool initialized = false;
};
