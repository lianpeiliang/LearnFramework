#pragma once

#include <QWidget>

class WidgetFactory;

// 内置计数面板；计数属于单个实例，容器重新创建该面板时从零开始。
class CWidget : public QWidget
{
public:
    explicit CWidget(QWidget *parent = nullptr);
private:
    int count = 0;
};

// 内置文本面板，将输入框的当前文字同步显示在标签中。
class DWidget : public QWidget
{
public:
    explicit DWidget(QWidget *parent = nullptr);
};

// 内置列表面板，提供无需插件即可展示的固定示例数据。
class EWidget : public QWidget
{
public:
    explicit EWidget(QWidget *parent = nullptr);
};

// 仅登记 c/d/e 的创建回调，实际窗口由容器按 contentClass 延迟创建。
void registerDemoWidgets(WidgetFactory &factory);
