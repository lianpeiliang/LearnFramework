#include "WidgetFactory.h"

void WidgetFactory::registerWidget(const QString &name, Creator creator)
{
    creators.insert(name, creator);
}

QWidget *WidgetFactory::create(const QString &name, QWidget *parent) const
{
    const auto it = creators.constFind(name);
    return it == creators.cend() || !it.value() ? nullptr : it.value()(parent);
}
