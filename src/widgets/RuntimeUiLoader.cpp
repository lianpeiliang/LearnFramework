#include "RuntimeUiLoader.h"
#include "WidgetContainer.h"

#include <QVariant>

RuntimeUiLoader::RuntimeUiLoader(const WidgetFactory &factory) : factory(factory)
{
}

QWidget *RuntimeUiLoader::createWidget(const QString &className, QWidget *parent,
                                      const QString &name)
{
    // Designer 提升控件时使用此类名；这里仅创建容器，动态属性稍后才会赋值。
    if (className == "WidgetContainer")
    {
        auto *container = new WidgetContainer(factory, parent);
        // 保留 .ui 中的对象名，供查找控件及错误提示使用。
        container->setObjectName(name);
        return container;
    }
    // 其他 Qt 控件继续使用 QUiLoader 的标准创建流程。
    return QUiLoader::createWidget(className, parent, name);
}

bool RuntimeUiLoader::populateContainers(QWidget *root, QString *error)
{
    // findChildren 递归查找后代但不包含 root，因此额外检查根是否也是容器。
    auto containers = root->findChildren<WidgetContainer *>();
    if (auto *container = qobject_cast<WidgetContainer *>(root))
        containers.prepend(container);

    // 必须在 load 完成后调用，此时 Designer 保存的 contentClass 动态属性已设置。
    // 顺序填充，首个失败通过 error 向调用方报告；此前完成的填充不会撤销。
    for (WidgetContainer *container : containers)
        if (!container->setContentClass(container->property("contentClass").toString(), error))
            return false;
    return true;
}
