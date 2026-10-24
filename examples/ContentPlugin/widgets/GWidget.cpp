#include "GWidget.h"
#include "DeviceStatusBinding.h"

#include <QLabel>
#include <QLineEdit>
#include <QVBoxLayout>

GWidget::GWidget(PluginServices *services, QWidget *parent) : QWidget(parent)
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
