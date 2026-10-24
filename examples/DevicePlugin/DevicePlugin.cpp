#include "DevicePlugin.h"

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
        const auto update = [state, name, status, interval, button] {
            name->setText(state->name);
            status->setText(state->connected ? QStringLiteral("状态：已连接（模拟）")
                                            : QStringLiteral("状态：未连接"));
            interval->setText(QStringLiteral("刷新间隔：%1 ms（配置演示）").arg(state->refreshInterval));
            button->setText(state->connected ? QStringLiteral("断开模拟设备")
                                            : QStringLiteral("连接模拟设备"));
        };
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
    if (!state)
        state = new DeviceState;
    qInfo() << "DevicePlugin initialized";
    return true;
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
    if (!state)
        return nullptr;
    if (name == "device.monitor")
        return new DeviceMonitorWidget(state, parent);
    if (name == "device.settings")
        return new DeviceSettingsWidget(state, parent);
    return nullptr;
}
