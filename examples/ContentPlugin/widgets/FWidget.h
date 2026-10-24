#pragma once

#include <QWidget>

class PluginServices;
class QPushButton;
class QLabel;

// 计数面板，同时演示通过主程序总线控制设备。
class FWidget : public QWidget
{
public:
    FWidget(PluginServices *services, QWidget *parent);

private:
    void setupUi();
    void connectSignals();
    void incrementCount();
    void requestDeviceConnection(bool connected);

    PluginServices *services;
    // 控件由 QWidget 父子关系管理，这里只保留借用指针。
    QPushButton *counterButton = nullptr;
    QPushButton *connectButton = nullptr;
    QPushButton *disconnectButton = nullptr;
    QLabel *commandResultLabel = nullptr;
    QLabel *deviceStatusLabel = nullptr;
    int count = 0;
};
