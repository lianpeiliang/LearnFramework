#include "FWidget.h"
#include "DeviceStatusBinding.h"
#include "DeviceProtocol.h"
#include "PluginServices.h"

#include <QHBoxLayout>
#include <QLabel>
#include <QPushButton>
#include <QVBoxLayout>

FWidget::FWidget(PluginServices *services, QWidget *parent)
    : QWidget(parent), services(services)
{
    setupUi();
    connectSignals();
    // 状态标签必须先创建；订阅与初始查询可能在此调用中立即更新标签。
    observeDevice(services, this, deviceStatusLabel);
}

void FWidget::setupUi()
{
    setObjectName("pluginFWidget");
    auto *layout = new QVBoxLayout(this);
    layout->addWidget(new QLabel(QStringLiteral("F：来自 ContentPlugin.dll"), this));
    counterButton = new QPushButton(QStringLiteral("插件计数：0"), this);
    counterButton->setObjectName("pluginCounterButton");
    layout->addWidget(counterButton);
    layout->addWidget(new QLabel(QStringLiteral("跨插件控制 → DevicePlugin"), this));
    auto *actions = new QHBoxLayout;
    connectButton = new QPushButton(QStringLiteral("发送连接命令"), this);
    connectButton->setObjectName("contentConnectDeviceButton");
    disconnectButton = new QPushButton(QStringLiteral("发送断开命令"), this);
    disconnectButton->setObjectName("contentDisconnectDeviceButton");
    connectButton->setEnabled(services != nullptr);
    disconnectButton->setEnabled(services != nullptr);
    actions->addWidget(connectButton);
    actions->addWidget(disconnectButton);
    layout->addLayout(actions);
    commandResultLabel = new QLabel(QStringLiteral("点击按钮，通过主程序总线发送命令。"), this);
    commandResultLabel->setObjectName("contentCommandResult");
    commandResultLabel->setWordWrap(true);
    deviceStatusLabel = new QLabel(this);
    deviceStatusLabel->setObjectName("contentDeviceStatus");
    layout->addWidget(commandResultLabel);
    layout->addWidget(deviceStatusLabel);
    layout->addStretch();
}

void FWidget::connectSignals()
{
    connect(counterButton, &QPushButton::clicked, this, &FWidget::incrementCount);
    connect(connectButton, &QPushButton::clicked, this, [this] { requestDeviceConnection(true); });
    connect(disconnectButton, &QPushButton::clicked, this, [this] { requestDeviceConnection(false); });
}

void FWidget::incrementCount()
{
    counterButton->setText(QStringLiteral("插件计数：%1").arg(++count));
}

void FWidget::requestDeviceConnection(bool connected)
{
    // 只发送协议约定的目标状态，不直接访问 DevicePlugin 的实现对象。
    QVariantMap data;
    data.insert("connected", connected);
    // 返回值表示命令是否被处理；设备的具体状态仍由订阅的广播消息刷新。
    const bool handled = services && services->sendCommand(DeviceProtocol::SetConnection, data);
    commandResultLabel->setText(handled ? QStringLiteral("命令已由 DevicePlugin 处理，状态消息已广播。")
                                       : QStringLiteral("命令未处理：设备服务不可用。"));
}
