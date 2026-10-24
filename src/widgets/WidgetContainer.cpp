#include "WidgetContainer.h"

#include <QVariant>
#include <QVBoxLayout>

WidgetContainer::WidgetContainer(const WidgetFactory &factory, QWidget *parent)
    : QWidget(parent), factory(factory), contentLayout(new QVBoxLayout(this))
{
    // 容器不额外留白，让业务窗口完整填充 Designer 分配给占位控件的区域。
    contentLayout->setContentsMargins(0, 0, 0, 0);
    contentLayout->setSpacing(0);
}

bool WidgetContainer::setContentClass(const QString &name, QString *error)
{
    if (error)
        error->clear();
    // 同名内容保留现有实例及其交互状态；空容器则仍继续同步属性。
    if (content && loadedClass == name)
        return true;

    // 先准备替代窗口；空名称表示主动清空，而非创建失败。
    QWidget *next = name.isEmpty() ? nullptr : factory.create(name, this);
    if (!name.isEmpty() && !next)
    {
        if (error)
            *error = QStringLiteral("Container '%1': unknown contentClass '%2' or creator failed")
                         .arg(objectName(), name);
        return false;
    }

    // 新内容创建成功后才替换；失败时保留当前界面。
    if (content)
    {
        contentLayout->removeWidget(content);
        delete content;
    }
    content = next;
    loadedClass = name;
    // 成功后同步动态属性，使后续查询看到实际生效的内容名称。
    setProperty("contentClass", name);
    if (content)
    {
        contentLayout->addWidget(content);
        content->show();
    }
    return true;
}

QWidget *WidgetContainer::contentWidget() const
{
    return content;
}
