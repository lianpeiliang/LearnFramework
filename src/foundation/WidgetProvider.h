#pragma once

#include <QStringList>
#include <QtPlugin>

class QWidget;

// 可选插件能力：只依赖公共接口和 Qt，不依赖主程序或窗口工厂实现。
class WidgetProvider
{
public:
    virtual ~WidgetProvider() = default;
    virtual QStringList widgetClasses() const = 0;
    // 每次返回新窗口，parent 拥有窗口；nullptr 表示创建失败。
    virtual QWidget *createWidget(const QString &name, QWidget *parent) = 0;
};

Q_DECLARE_INTERFACE(WidgetProvider, "org.learnframework.WidgetProvider/1.0")
