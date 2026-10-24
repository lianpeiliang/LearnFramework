#include "ContentPlugin.h"
#include "widgets/FWidget.h"
#include "widgets/GWidget.h"

#include <QDebug>

bool ContentPlugin::initialize()
{
    initialized = true;
    qInfo() << "ContentPlugin initialized";
    return true;
}

bool ContentPlugin::shutdown()
{
    if (initialized)
        qInfo() << "ContentPlugin shutdown";
    initialized = false;
    return true;
}

QStringList ContentPlugin::widgetClasses() const
{
    return {QStringLiteral("plugin.f"), QStringLiteral("plugin.g")};
}

QWidget *ContentPlugin::createWidget(const QString &name, QWidget *parent)
{
    if (!initialized)
        return nullptr;
    if (name == "plugin.f")
        return new FWidget(services, parent);
    if (name == "plugin.g")
        return new GWidget(services, parent);
    return nullptr;
}
