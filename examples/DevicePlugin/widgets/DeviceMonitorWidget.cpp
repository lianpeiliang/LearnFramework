#include "DeviceMonitorWidget.h"
#include "../DeviceState.h"

#include <QLabel>
#include <QPushButton>
#include <QVBoxLayout>

DeviceMonitorWidget::DeviceMonitorWidget(DeviceState *state, QWidget *parent)
    : QWidget(parent)
{
    setObjectName("deviceMonitorWidget");
    auto *layout = new QVBoxLayout(this);
    layout->addWidget(new QLabel(QStringLiteral("DevicePlugin.dll · 模拟设备"), this));
    auto *name = new QLabel(this);
    name->setObjectName("deviceNameLabel");
    auto *status = new QLabel(this);
    status->setObjectName("deviceStatusLabel");
    auto *interval = new QLabel(this);
    interval->setObjectName("deviceIntervalLabel");
    auto *button = new QPushButton(this);
    button->setObjectName("deviceConnectionButton");
    layout->addWidget(name);
    layout->addWidget(status);
    layout->addWidget(interval);
    layout->addWidget(button);
    layout->addStretch();
    // 本地面板操作与跨插件命令最终都修改同一份状态，监控界面统一在此刷新。
    const auto update = [state, name, status, interval, button] {
        name->setText(state->name);
        status->setText(state->connected ? QStringLiteral("状态：已连接（模拟）")
                                        : QStringLiteral("状态：未连接"));
        interval->setText(QStringLiteral("刷新间隔：%1 ms（配置演示）").arg(state->refreshInterval));
        button->setText(state->connected ? QStringLiteral("断开模拟设备")
                                        : QStringLiteral("连接模拟设备"));
    };
    // 以面板为连接上下文，面板销毁后不再访问其标签和按钮。
    connect(state, &DeviceState::changed, this, update);
    connect(button, &QPushButton::clicked, this, [state] {
        state->connected = !state->connected;
        emit state->changed();
    });
    update();
}
