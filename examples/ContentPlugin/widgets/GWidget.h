#pragma once

#include <QWidget>

class PluginServices;

// 文本输入面板，独立订阅设备状态。
class GWidget : public QWidget
{
public:
    GWidget(PluginServices *services, QWidget *parent);
};
