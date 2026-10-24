#pragma once

#include <QStringList>
#include <QtPlugin>

class QWidget;

// 可选插件能力：只依赖公共接口和 Qt，不依赖主程序或窗口工厂实现。
class WidgetProvider
{
public:
    virtual ~WidgetProvider() = default;
    // 返回可供工厂登记的类型名，需在本插件内唯一且不与其他已注册名称冲突。
    // 加载器会拒绝空白名称；这些名称也会原样传给 createWidget。
    virtual QStringList widgetClasses() const = 0;
    // 每次返回新窗口，非空 parent 拥有窗口；无 parent 时调用方负责释放。
    // 不支持的名称或创建失败时返回 nullptr，不能返回共享的已有窗口。
    virtual QWidget *createWidget(const QString &name, QWidget *parent) = 0;
};

Q_DECLARE_INTERFACE(WidgetProvider, "org.learnframework.WidgetProvider/1.0")
