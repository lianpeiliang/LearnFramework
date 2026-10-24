#include "DevicePlugin.h"
#include "DeviceProtocol.h"

#include <QDebug>
#include <QFormLayout>
#include <QLabel>
#include <QLineEdit>
#include <QPushButton>
#include <QSpinBox>
#include <QVBoxLayout>
#include <QWidget>

class DeviceMonitorWidget : public QWidget
{
public:
    DeviceMonitorWidget(DeviceState *state, QWidget *parent) : QWidget(parent)
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
};

class DeviceSettingsWidget : public QWidget
{
public:
    DeviceSettingsWidget(DeviceState *state, QWidget *parent) : QWidget(parent)
    {
        setObjectName("deviceSettingsWidget");
        auto *layout = new QVBoxLayout(this);
        layout->addWidget(new QLabel(QStringLiteral("配置会同步到设备监控面板"), this));
        auto *form = new QFormLayout;
        auto *name = new QLineEdit(state->name, this);
        name->setObjectName("deviceNameInput");
        auto *interval = new QSpinBox(this);
        interval->setObjectName("deviceIntervalInput");
        interval->setRange(100, 60000);
        interval->setSuffix(" ms");
        interval->setValue(state->refreshInterval);
        form->addRow(QStringLiteral("设备名称"), name);
        form->addRow(QStringLiteral("刷新间隔"), interval);
        layout->addLayout(form);
        auto *apply = new QPushButton(QStringLiteral("应用配置"), this);
        apply->setObjectName("deviceApplyButton");
        auto *message = new QLabel(this);
        message->setWordWrap(true);
        layout->addWidget(apply);
        layout->addWidget(message);
        layout->addStretch();
        // 输入框先保留草稿，校验通过并点击应用后才更新共享状态。
        connect(apply, &QPushButton::clicked, this, [state, name, interval, message] {
            if (name->text().trimmed().isEmpty())
            {
                message->setText(QStringLiteral("请输入设备名称。"));
                return;
            }
            state->name = name->text().trimmed();
            state->refreshInterval = interval->value();
            emit state->changed();
            message->setText(QStringLiteral("已应用，仅在本次运行中保留。"));
        });
    }
};

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
