#pragma once

#include <QWidget>

class WidgetFactory;

class CWidget : public QWidget
{
public:
    explicit CWidget(QWidget *parent = nullptr);
private:
    int count = 0;
};

class DWidget : public QWidget
{
public:
    explicit DWidget(QWidget *parent = nullptr);
};

class EWidget : public QWidget
{
public:
    explicit EWidget(QWidget *parent = nullptr);
};

void registerDemoWidgets(WidgetFactory &factory);
