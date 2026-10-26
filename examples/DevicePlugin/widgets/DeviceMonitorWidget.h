#pragma once

#include <QWidget>

class DeviceState;

class DeviceMonitorWidget : public QWidget
{
public:
    DeviceMonitorWidget(DeviceState *state, QWidget *parent);
};
