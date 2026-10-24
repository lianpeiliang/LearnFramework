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
    // 每个面板独立订阅；这些动态属性记录本标签收到的次数与最近一次设备状态。
    services->subscribe(DeviceProtocol::StatusChanged, owner, [status](const QVariantMap &data) {
        const int count = status->property("receivedEvents").toInt() + 1;
        const bool connected = data.value("connected").toBool();
        status->setProperty("receivedEvents", count);
        status->setProperty("connected", connected);
        status->setProperty("deviceName", data.value("name"));
        status->setText(QStringLiteral("消息 #%1：%2，%3").arg(count).arg(data.value("name").toString())
                            .arg(connected ? QStringLiteral("已连接（模拟）") : QStringLiteral("未连接")));
    });
    // 先订阅再主动查询，确保新打开的面板立即拿到快照，即使设备尚未发生变化。
    // 查询的回复也是广播，已打开的其他面板同样会收到，消息计数随之增加。
    if (!services->sendCommand(DeviceProtocol::QueryStatus))
        status->setText(QStringLiteral("设备服务不可用：请检查 DevicePlugin.dll"));
}
