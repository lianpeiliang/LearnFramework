#pragma once

#include <QWidget>

class DeviceState;

class DeviceSettingsWidget : public QWidget
{
public:
    DeviceSettingsWidget(DeviceState *state, QWidget *parent);
};
