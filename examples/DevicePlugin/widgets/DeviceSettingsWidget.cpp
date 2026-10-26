#include "DeviceSettingsWidget.h"
#include "../DeviceState.h"

#include <QLabel>
#include <QPushButton>
#include <QVBoxLayout>
#include <QFormLayout>
#include <QLineEdit>
#include <QSpinBox>

DeviceSettingsWidget::DeviceSettingsWidget(DeviceState *state, QWidget *parent)
    : QWidget(parent), state(state)
{
    setupUi();
    loadSettings();
    connectSignals();
}

void DeviceSettingsWidget::setupUi()
{
    setObjectName("deviceSettingsWidget");
    auto *layout = new QVBoxLayout(this);
    layout->addWidget(new QLabel(QStringLiteral("配置会同步到设备监控面板"), this));
    auto *form = new QFormLayout;
    nameInput = new QLineEdit(this);
    nameInput->setObjectName("deviceNameInput");
    intervalInput = new QSpinBox(this);
    intervalInput->setObjectName("deviceIntervalInput");
    intervalInput->setRange(100, 60000);
    intervalInput->setSuffix(" ms");
    form->addRow(QStringLiteral("设备名称"), nameInput);
    form->addRow(QStringLiteral("刷新间隔"), intervalInput);
    layout->addLayout(form);
    applyButton = new QPushButton(QStringLiteral("应用配置"), this);
    applyButton->setObjectName("deviceApplyButton");
    messageLabel = new QLabel(this);
    messageLabel->setWordWrap(true);
    layout->addWidget(applyButton);
    layout->addWidget(messageLabel);
    layout->addStretch();
}

void DeviceSettingsWidget::loadSettings()
{
    // 仅在创建时读取初值，后续状态通知不覆盖用户尚未应用的草稿。
    nameInput->setText(state->name);
    intervalInput->setValue(state->refreshInterval);
}

void DeviceSettingsWidget::connectSignals()
{
    connect(applyButton, &QPushButton::clicked, this, &DeviceSettingsWidget::applySettings);
}

void DeviceSettingsWidget::applySettings()
{
    const QString name = nameInput->text().trimmed();
    if (name.isEmpty())
    {
        messageLabel->setText(QStringLiteral("请输入设备名称。"));
        return;
    }

    state->name = name;
    state->refreshInterval = intervalInput->value();
    emit state->changed();
    messageLabel->setText(QStringLiteral("已应用，仅在本次运行中保留。"));
}
