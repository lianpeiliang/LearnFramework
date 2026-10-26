#pragma once

#include <QWidget>

class DeviceState;
class QLabel;
class QPushButton;

class DeviceMonitorWidget : public QWidget
{
public:
    DeviceMonitorWidget(DeviceState *state, QWidget *parent);

private:
    void setupUi();
    void connectSignals();
    void refreshState();
    void toggleConnection();

    QLabel *nameLabel = nullptr;
    QLabel *statusLabel = nullptr;
    QLabel *intervalLabel = nullptr;
    QPushButton *connectionButton = nullptr;
    // 借用插件拥有的共享状态；插件关闭前会先销毁面板。
    DeviceState *state;
};
