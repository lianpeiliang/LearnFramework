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
    // 注册布局配置可使用的类型键；这里只声明类型，不提前创建 QWidget。
    return {QStringLiteral("plugin.f"), QStringLiteral("plugin.g")};
}

QWidget *ContentPlugin::createWidget(const QString &name, QWidget *parent)
{
    // 工厂只在初始化成功后工作，并把服务和父窗口传给每个新控件实例。
    if (!initialized)
        return nullptr;
    if (name == "plugin.f")
        return new FWidget(services, parent);
    if (name == "plugin.g")
        return new GWidget(services, parent);
    return nullptr;
}
