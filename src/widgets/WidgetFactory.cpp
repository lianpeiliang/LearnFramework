#include "WidgetFactory.h"

bool WidgetFactory::registerWidget(const QString &name, Creator creator)
{
    if (name.trimmed().isEmpty() || !creator || contains(name))
        return false;
    // 保存回调副本而非立即创建窗口；其按值捕获的资源随回调一起保留。
    creators.insert(name, creator);
    return true;
}

bool WidgetFactory::contains(const QString &name) const
{
    return creators.contains(name);
}

void WidgetFactory::clear()
{
    creators.clear();
}

QWidget *WidgetFactory::create(const QString &name, QWidget *parent) const
{
    // 只查询已有登记；constFind 不会为未知名称插入一个空回调。
    QHash<QString, Creator>::const_iterator iterator = creators.constFind(name);
    if (iterator == creators.cend())
    {
        return nullptr;
    }

    // 借用工厂内的回调，避免本次创建额外复制其捕获的状态。
    const Creator &creator = iterator.value();
    if (!creator)
    {
        return nullptr;
    }

    // parent 交给具体回调处理；窗口归属由回调创建时建立的 Qt 父子关系决定。
    QWidget *widget = creator(parent);
    return widget;
}
