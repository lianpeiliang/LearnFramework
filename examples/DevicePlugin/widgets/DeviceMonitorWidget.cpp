#include "DeviceMonitorWidget.h"
#include "../DeviceState.h"

#include <QLabel>
#include <QPushButton>
#include <QVBoxLayout>

DeviceMonitorWidget::DeviceMonitorWidget(DeviceState *state, QWidget *parent)
    : QWidget(parent), state(state)
{
    setupUi();
    connectSignals();
    refreshState();
}

void DeviceMonitorWidget::setupUi()
{
    setObjectName("deviceMonitorWidget");
    auto *layout = new QVBoxLayout(this);
    layout->addWidget(new QLabel(QStringLiteral("DevicePlugin.dll · 模拟设备"), this));
    nameLabel = new QLabel(this);
    nameLabel->setObjectName("deviceNameLabel");
    statusLabel = new QLabel(this);
    statusLabel->setObjectName("deviceStatusLabel");
    intervalLabel = new QLabel(this);
    intervalLabel->setObjectName("deviceIntervalLabel");
    connectionButton = new QPushButton(this);
    connectionButton->setObjectName("deviceConnectionButton");
    layout->addWidget(nameLabel);
    layout->addWidget(statusLabel);
    layout->addWidget(intervalLabel);
    layout->addWidget(connectionButton);
    layout->addStretch();
}

void DeviceMonitorWidget::connectSignals()
{
    // 以面板为连接上下文，面板销毁后自动断开。
    connect(state, &DeviceState::changed, this, &DeviceMonitorWidget::refreshState);
    connect(connectionButton, &QPushButton::clicked, this, &DeviceMonitorWidget::toggleConnection);
}

void DeviceMonitorWidget::refreshState()
{
    // 本地操作和跨插件命令修改同一份状态，界面统一在此刷新。
    nameLabel->setText(state->name);
    statusLabel->setText(state->connected ? QStringLiteral("状态：已连接（模拟）")
                                         : QStringLiteral("状态：未连接"));
    intervalLabel->setText(QStringLiteral("刷新间隔：%1 ms（配置演示）").arg(state->refreshInterval));
    connectionButton->setText(state->connected ? QStringLiteral("断开模拟设备")
                                              : QStringLiteral("连接模拟设备"));
}

void DeviceMonitorWidget::toggleConnection()
{
    state->connected = !state->connected;
    emit state->changed();
}
