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
    if (!registerCommands())
        return false;
    qInfo() << "DevicePlugin initialized";
    return true;
}

bool DevicePlugin::registerCommands()
{
    // 没有通信服务时，仍允许使用本地设备面板。
    if (!services)
        return true;

    // Lambda 只负责转交参数，具体处理逻辑放在对应的成员方法中。
    const bool connectionRegistered = services->registerCommand(
        DeviceProtocol::SetConnection, this,
        [this](const QVariantMap &data) { return handleSetConnection(data); });
    if (!connectionRegistered)
    {
        qWarning() << "DevicePlugin: failed to register SetConnection (2000)";
        return false;
    }

    const bool queryRegistered = services->registerCommand(
        DeviceProtocol::QueryStatus, this,
        [this](const QVariantMap &) { return handleQueryStatus(); });
    if (!queryRegistered)
    {
        // 初始化失败后，加载器负责清理先前注册成功的命令。
        qWarning() << "DevicePlugin: failed to register QueryStatus (2001)";
        return false;
    }

    return true;
}

bool DevicePlugin::handleSetConnection(const QVariantMap &data)
{
    if (!state)
        return false;

    const QVariant connected = data.value("connected");
    if (connected.type() != QVariant::Bool)
        return false;

    state->connected = connected.toBool();
    emit state->changed();
    return true;
}

bool DevicePlugin::handleQueryStatus()
{
    if (!state)
        return false;

    // 查询不改变设备，通过状态消息回复所有订阅者。
    publishState();
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
