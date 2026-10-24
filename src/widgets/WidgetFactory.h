#pragma once

#include <QHash>
#include <QString>
#include <functional>

class QWidget;

// 类名到构造函数的映射；每次创建一个独立实例，所有权交给 parent。
class WidgetFactory
{
public:
    using Creator = std::function<QWidget *(QWidget *parent)>;
    void registerWidget(const QString &name, Creator creator);
    QWidget *create(const QString &name, QWidget *parent) const;

private:
    QHash<QString, Creator> creators;
};
