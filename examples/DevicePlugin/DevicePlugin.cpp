#include "DevicePlugin.h"
#include "DeviceState.h"
#include "DeviceProtocol.h"
#include "widgets/DeviceMonitorWidget.h"
#include "widgets/DeviceSettingsWidget.h"

#include <QDebug>

bool DevicePlugin::initialize()
{
    if (state)
        return true;
    state = new DeviceState;
    // 把插件内部的 Qt 状态通知转换为主程序总线消息，供 ContentPlugin 等订阅。
    connect(state, &DeviceState::changed, this, [this] { publishState(); });
    if (services)
    {
        // SetConnection 是带布尔载荷的控制命令；拒绝缺少字段或类型错误的请求。
        // 命令属于当前插件服务作用域，加载器关闭插件时会统一撤销注册。
        if (!services->registerCommand(DeviceProtocol::SetConnection, this, [this](const QVariantMap &data) {
                if (!state || data.value("connected").type() != QVariant::Bool)
                    return false;
                state->connected = data.value("connected").toBool();
                emit state->changed();
                return true;
            }) ||
            // 查询不修改设备，使用同一状态消息回复，因此所有订阅者都能收到。
            !services->registerCommand(DeviceProtocol::QueryStatus, this, [this](const QVariantMap &) {
                if (!state)
                    return false;
                publishState();
                return true;
            }))
        {
            // 返回失败后，加载器仍会调用 shutdown 并清理已成功注册的部分命令。
            qWarning() << "DevicePlugin command registration conflict";
            return false;
        }
    }
    qInfo() << "DevicePlugin initialized";
    return true;
}

void DevicePlugin::publishState()
{
    if (!services || !state)
        return;
    // 发送完整状态快照，接收方无须依赖本插件的 QObject 或内部结构。
    QVariantMap data;
    data.insert("connected", state->connected);
    data.insert("name", state->name);
    data.insert("refreshInterval", state->refreshInterval);
    services->publish(DeviceProtocol::StatusChanged, data);
}

bool DevicePlugin::shutdown()
{
    delete state;
    state = nullptr;
    qInfo() << "DevicePlugin shutdown";
    return true;
}

QStringList DevicePlugin::widgetClasses() const
{
    return {QStringLiteral("device.monitor"), QStringLiteral("device.settings")};
}

QWidget *DevicePlugin::createWidget(const QString &name, QWidget *parent)
{
    // 控件可以反复创建；各实例共享设备状态，父对象负责各自的界面生命周期。
    if (!state)
        return nullptr;
    if (name == "device.monitor")
        return new DeviceMonitorWidget(state, parent);
    if (name == "device.settings")
        return new DeviceSettingsWidget(state, parent);
    return nullptr;
}
