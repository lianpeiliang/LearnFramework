#include "WidgetContainer.h"

#include <QVariant>
#include <QVBoxLayout>

WidgetContainer::WidgetContainer(const WidgetFactory &factory, QWidget *parent)
    : QWidget(parent), factory(factory), contentLayout(new QVBoxLayout(this))
{
    contentLayout->setContentsMargins(0, 0, 0, 0);
    contentLayout->setSpacing(0);
}

bool WidgetContainer::setContentClass(const QString &name, QString *error)
{
    if (error)
        error->clear();
    if (content && loadedClass == name)
        return true;

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
