#include "WidgetFactory.h"

bool WidgetFactory::registerWidget(const QString &name, Creator creator)
{
    if (name.trimmed().isEmpty() || !creator || contains(name))
        return false;
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
    // 第一步：按名称查找，末尾迭代器表示没有登记这个名称。
    QHash<QString, Creator>::const_iterator iterator = creators.constFind(name);
    if (iterator == creators.cend())
    {
        return nullptr;
    }

    // 第二步：取出登记的创建函数，并检查它是否有效。
    const Creator &creator = iterator.value();
    if (!creator)
    {
        return nullptr;
    }

    // 第三步：调用创建函数，把父窗口传进去，返回实际创建的窗口。
    QWidget *widget = creator(parent);
    return widget;
}
