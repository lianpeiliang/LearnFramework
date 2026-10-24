#pragma once

#include "Plugin.h"
#include "WidgetProvider.h"
#include <QObject>

class ReportState : public QObject
{
    Q_OBJECT
public:
    QString text = QStringLiteral("示例报表\n\n在下方编辑器修改文字，然后点击“更新预览”。");

signals:
    void changed();
};

class ReportPlugin : public QObject, public Plugin, public WidgetProvider
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
    ReportState *state = nullptr;
};
