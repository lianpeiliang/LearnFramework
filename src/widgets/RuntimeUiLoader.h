#pragma once

#include "WidgetFactory.h"
#include <QUiLoader>

class RuntimeUiLoader : public QUiLoader
{
public:
    explicit RuntimeUiLoader(const WidgetFactory &factory);
    bool populateContainers(QWidget *root, QString *error);

protected:
    QWidget *createWidget(const QString &className, QWidget *parent,
                          const QString &name) override;

private:
    const WidgetFactory &factory;
};
