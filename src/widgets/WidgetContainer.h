#pragma once

#include "WidgetFactory.h"
#include <QWidget>

class QVBoxLayout;

// Designer 中的占位容器：读取 contentClass 指定的名称，承载工厂创建的业务窗口。
class WidgetContainer : public QWidget
{
    Q_OBJECT

public:
    explicit WidgetContainer(const WidgetFactory &factory, QWidget *parent = nullptr);
    // 空名称清空内容；非空名称创建失败时保留原窗口，并通过可选 error 返回原因。
    bool setContentClass(const QString &name, QString *error = nullptr);
    // 返回借用指针；切换内容或销毁容器后，该指针可能失效。
    QWidget *contentWidget() const;

private:
    // 保存构造时的工厂副本，因此应先完成类型注册，再创建容器。
    WidgetFactory factory;
    QVBoxLayout *contentLayout;
    // content 以本容器为父对象；替换时主动删除，容器销毁时由 Qt 回收。
    QWidget *content = nullptr;
    // 记录已成功应用的名称，用于避免重复创建同一个内容。
    QString loadedClass;
};
