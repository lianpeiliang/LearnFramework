#include "DeviceSettingsWidget.h"
#include "../DeviceState.h"

#include <QLabel>
#include <QPushButton>
#include <QVBoxLayout>
#include <QFormLayout>
#include <QLineEdit>
#include <QSpinBox>

DeviceSettingsWidget::DeviceSettingsWidget(DeviceState *state, QWidget *parent)
    : QWidget(parent)
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
