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
    const auto it = creators.constFind(name);
    return it == creators.cend() || !it.value() ? nullptr : it.value()(parent);
}
