#pragma once

#include "Plugin.h"
#include "WidgetProvider.h"
#include <QObject>

// 插件内所有编辑器与预览共享一份已应用的报表文本，不写入磁盘。
class ReportState : public QObject
{
    Q_OBJECT
public:
    QString text = QStringLiteral("示例报表\n\n在下方编辑器修改文字，然后点击“更新预览”。");

signals:
    void changed();
};

// 在必需的生命周期接口之外，通过 WidgetProvider 提供两类可嵌入主界面的控件。
class ReportPlugin : public QObject, public Plugin, public WidgetProvider
{
    Q_OBJECT
    // 插件入口元数据与接口声明分别供 Qt 加载及 qobject_cast 查询使用。
    Q_PLUGIN_METADATA(IID "org.learnframework.Plugin/1.0")
    Q_INTERFACES(Plugin WidgetProvider)
public:
    bool initialize() override;
    // 该示例没有单独的启停任务；界面由主程序按需创建和显示。
    bool start() override { return true; }
    bool stop() override { return true; }
    bool shutdown() override;
    QStringList widgetClasses() const override;
    QWidget *createWidget(const QString &name, QWidget *parent) override;

private:
    // 插件拥有共享状态；主程序先销毁插件创建的控件，再调用 shutdown 释放它。
    ReportState *state = nullptr;
};
