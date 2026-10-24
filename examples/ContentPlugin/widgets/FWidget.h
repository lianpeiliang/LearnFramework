#pragma once

#include <QWidget>

class PluginServices;

// 计数面板，同时演示通过主程序总线控制设备。
class FWidget : public QWidget
{
public:
    FWidget(PluginServices *services, QWidget *parent);

private:
    int count = 0;
};
