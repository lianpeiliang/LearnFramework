#include "ContentPlugin.h"
#include "DeviceProtocol.h"

#include <QDebug>
#include <QHBoxLayout>
#include <QLabel>
#include <QLineEdit>
#include <QPushButton>
#include <QVBoxLayout>
#include <QWidget>

namespace
{
void observeDevice(PluginServices *services, QWidget *owner, QLabel *status)
{
    status->setWordWrap(true);
    status->setText(QStringLiteral("设备状态：等待消息"));
    if (!services)
    {
        status->setText(QStringLiteral("当前主程序未提供插件通信接口"));
        return;
    }
    services->subscribe(DeviceProtocol::StatusChanged, owner, [status](const QVariantMap &data) {
        const int count = status->property("receivedEvents").toInt() + 1;
        const bool connected = data.value("connected").toBool();
        status->setProperty("receivedEvents", count);
        status->setProperty("connected", connected);
        status->setProperty("deviceName", data.value("name"));
        status->setText(QStringLiteral("消息 #%1：%2，%3").arg(count).arg(data.value("name").toString())
                            .arg(connected ? QStringLiteral("已连接（模拟）") : QStringLiteral("未连接")));
    });
    if (!services->sendCommand(DeviceProtocol::QueryStatus))
        status->setText(QStringLiteral("设备服务不可用：请检查 DevicePlugin.dll"));
}
}

// 这两个业务类只编译在 DLL 中；主程序没有包含或链接它们。
class FWidget : public QWidget
{
public:
    FWidget(PluginServices *services, QWidget *parent) : QWidget(parent)
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

private:
    int count = 0;
};

class GWidget : public QWidget
{
public:
    GWidget(PluginServices *services, QWidget *parent) : QWidget(parent)
    {
        setObjectName("pluginGWidget");
        auto *layout = new QVBoxLayout(this);
        layout->addWidget(new QLabel(QStringLiteral("G：来自 ContentPlugin.dll"), this));
        auto *input = new QLineEdit(this);
        input->setObjectName("pluginTextInput");
        input->setPlaceholderText(QStringLiteral("输入文字，体验 DLL 内的界面逻辑"));
        auto *label = new QLabel(this);
        label->setObjectName("pluginTextLabel");
        layout->addWidget(input);
        layout->addWidget(label);
        layout->addWidget(new QLabel(QStringLiteral("独立订阅设备状态消息"), this));
        auto *status = new QLabel(this);
        status->setObjectName("contentTextDeviceStatus");
        layout->addWidget(status);
        observeDevice(services, this, status);
        layout->addStretch();
        connect(input, &QLineEdit::textChanged, label, &QLabel::setText);
    }
};

bool ContentPlugin::initialize()
{
    initialized = true;
    qInfo() << "ContentPlugin initialized";
    return true;
}

bool ContentPlugin::shutdown()
{
    if (initialized)
        qInfo() << "ContentPlugin shutdown";
    initialized = false;
    return true;
}

QStringList ContentPlugin::widgetClasses() const
{
    return {QStringLiteral("plugin.f"), QStringLiteral("plugin.g")};
}

QWidget *ContentPlugin::createWidget(const QString &name, QWidget *parent)
{
    if (!initialized)
        return nullptr;
    if (name == "plugin.f")
        return new FWidget(services, parent);
    if (name == "plugin.g")
        return new GWidget(services, parent);
    return nullptr;
}
