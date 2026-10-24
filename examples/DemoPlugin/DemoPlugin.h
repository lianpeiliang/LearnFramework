#pragma once

#include "Plugin.h"

#include <QObject>
#include <QPointer>
#include <QWidget>

// QObject 提供 Qt 元对象能力，Plugin 提供框架约定的生命周期接口。
class DemoPlugin : public QObject, public Plugin
{
    Q_OBJECT
    Q_PLUGIN_METADATA(IID "org.learnframework.Plugin/1.0")
    Q_INTERFACES(Plugin)

public:
    ~DemoPlugin() override;

    bool initialize() override;
    bool start() override;
    bool stop() override;
    bool shutdown() override;

private:
    QPointer<QWidget> window;
    int clickCount = 0;
};
