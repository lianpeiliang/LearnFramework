#include "RuntimeUiLoader.h"
#include "WidgetContainer.h"

#include <QVariant>

RuntimeUiLoader::RuntimeUiLoader(const WidgetFactory &factory) : factory(factory)
{
}

QWidget *RuntimeUiLoader::createWidget(const QString &className, QWidget *parent,
                                      const QString &name)
{
    if (className == "WidgetContainer")
    {
        auto *container = new WidgetContainer(factory, parent);
        container->setObjectName(name);
        return container;
    }
    return QUiLoader::createWidget(className, parent, name);
}

bool RuntimeUiLoader::populateContainers(QWidget *root, QString *error)
{
    auto containers = root->findChildren<WidgetContainer *>();
    if (auto *container = qobject_cast<WidgetContainer *>(root))
        containers.prepend(container);

    // load 完成后，Designer 保存的动态属性才已设置，可以据此创建内容。
    for (WidgetContainer *container : containers)
        if (!container->setContentClass(container->property("contentClass").toString(), error))
            return false;
    return true;
}
