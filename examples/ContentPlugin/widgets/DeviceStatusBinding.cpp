#include "DeviceStatusBinding.h"
#include "DeviceProtocol.h"
#include "PluginServices.h"

#include <QLabel>
#include <QWidget>

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
