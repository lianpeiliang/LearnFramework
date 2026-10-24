#pragma once

#include "Plugin.h"

#include <QObject>
#include <QPointer>
#include <QWidget>

// QObject 提供 Qt 元对象能力，Plugin 提供框架约定的生命周期接口。
// 此示例自己管理独立工具窗口，不实现 WidgetProvider，也不向主界面工厂注册控件。
class DemoPlugin : public QObject, public Plugin
{
    Q_OBJECT
    // 元数据让 QPluginLoader 识别入口；Q_INTERFACES 让主程序能查询 Plugin 接口。
    Q_PLUGIN_METADATA(IID "org.learnframework.Plugin/1.0")
    Q_INTERFACES(Plugin)

public:
    ~DemoPlugin() override;

    bool initialize() override;
    bool start() override;
    bool stop() override;
    bool shutdown() override;

private:
    // 弱引用：窗口被外部销毁时自动置空，关闭插件时可安全判断是否仍需释放。
    QPointer<QWidget> window;
    int clickCount = 0;
};
