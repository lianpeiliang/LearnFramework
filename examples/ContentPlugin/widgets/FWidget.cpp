#include "FWidget.h"
#include "DeviceStatusBinding.h"
#include "DeviceProtocol.h"
#include "PluginServices.h"

#include <QHBoxLayout>
#include <QLabel>
#include <QPushButton>
#include <QVBoxLayout>

FWidget::FWidget(PluginServices *services, QWidget *parent) : QWidget(parent)
{
    setObjectName("pluginFWidget");
    auto *layout = new QVBoxLayout(this);
    layout->addWidget(new QLabel(QStringLiteral("F：来自 ContentPlugin.dll"), this));
    auto *button = new QPushButton(QStringLiteral("插件计数：0"), this);
    button->setObjectName("pluginCounterButton");
    layout->addWidget(button);
    layout->addWidget(new QLabel(QStringLiteral("跨插件控制 → DevicePlugin"), this));
    auto *actions = new QHBoxLayout;
    auto *connectDevice = new QPushButton(QStringLiteral("发送连接命令"), this);
    connectDevice->setObjectName("contentConnectDeviceButton");
    auto *disconnectDevice = new QPushButton(QStringLiteral("发送断开命令"), this);
    disconnectDevice->setObjectName("contentDisconnectDeviceButton");
    connectDevice->setEnabled(services != nullptr);
    disconnectDevice->setEnabled(services != nullptr);
    actions->addWidget(connectDevice);
    actions->addWidget(disconnectDevice);
    layout->addLayout(actions);
    auto *result = new QLabel(QStringLiteral("点击按钮，通过主程序总线发送命令。"), this);
    result->setObjectName("contentCommandResult");
    result->setWordWrap(true);
    auto *status = new QLabel(this);
    status->setObjectName("contentDeviceStatus");
    layout->addWidget(result);
    layout->addWidget(status);
    const auto request = [services, result](bool connected) {
        QVariantMap data;
        data.insert("connected", connected);
        const bool handled = services && services->sendCommand(DeviceProtocol::SetConnection, data);
        result->setText(handled ? QStringLiteral("命令已由 DevicePlugin 处理，状态消息已广播。")
                                : QStringLiteral("命令未处理：设备服务不可用。"));
    };
    connect(connectDevice, &QPushButton::clicked, this, [request] { request(true); });
    connect(disconnectDevice, &QPushButton::clicked, this, [request] { request(false); });
    observeDevice(services, this, status);
    layout->addStretch();
    connect(button, &QPushButton::clicked, this, [this, button] {
        button->setText(QStringLiteral("插件计数：%1").arg(++count));
    });
}
