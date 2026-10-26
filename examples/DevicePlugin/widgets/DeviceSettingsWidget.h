#pragma once

#include <QWidget>

class DeviceState;
class QLabel;
class QLineEdit;
class QPushButton;
class QSpinBox;

class DeviceSettingsWidget : public QWidget
{
public:
    DeviceSettingsWidget(DeviceState *state, QWidget *parent);

private:
    void setupUi();
    void connectSignals();
    void loadSettings();
    void applySettings();

    QLineEdit *nameInput = nullptr;
    QSpinBox *intervalInput = nullptr;
    QPushButton *applyButton = nullptr;
    QLabel *messageLabel = nullptr;
    // 借用插件拥有的共享状态；插件关闭前会先销毁面板。
    DeviceState *state;
};
