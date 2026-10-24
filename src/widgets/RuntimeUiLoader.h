#pragma once

#include "WidgetFactory.h"
#include <QUiLoader>

// 为 QUiLoader 补充 WidgetContainer 类型，并在 UI 加载完成后填充容器内容。
class RuntimeUiLoader : public QUiLoader
{
public:
    explicit RuntimeUiLoader(const WidgetFactory &factory);
    // root 必须有效；包含根容器本身，任一容器失败即停止，不回滚此前的填充。
    bool populateContainers(QWidget *root, QString *error);

protected:
    QWidget *createWidget(const QString &className, QWidget *parent,
                          const QString &name) override;

private:
    // 借用外部工厂，须在加载器使用期间有效；创建出的容器会自行保存工厂副本。
    const WidgetFactory &factory;
};
