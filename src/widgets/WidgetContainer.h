#pragma once

#include "WidgetFactory.h"
#include <QWidget>

class QVBoxLayout;

// 对应示意中的 b：只负责承载内容，业务界面由工厂创建。
class WidgetContainer : public QWidget
{
    Q_OBJECT

public:
    explicit WidgetContainer(const WidgetFactory &factory, QWidget *parent = nullptr);
    bool setContentClass(const QString &name, QString *error = nullptr);
    QWidget *contentWidget() const;

private:
    WidgetFactory factory;
    QVBoxLayout *contentLayout;
    QWidget *content = nullptr;
    QString loadedClass;
};
