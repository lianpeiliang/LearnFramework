#include "GWidget.h"
#include "DeviceStatusBinding.h"

#include <QLabel>
#include <QLineEdit>
#include <QVBoxLayout>

GWidget::GWidget(PluginServices *services, QWidget *parent) : QWidget(parent)
{
    setupUi();
    connectSignals();
    observeDevice(services, this, deviceStatusLabel);
}

void GWidget::setupUi()
{
    setObjectName("pluginGWidget");
    auto *layout = new QVBoxLayout(this);
    layout->addWidget(new QLabel(QStringLiteral("G：来自 ContentPlugin.dll"), this));
    textInput = new QLineEdit(this);
    textInput->setObjectName("pluginTextInput");
    textInput->setPlaceholderText(QStringLiteral("输入文字，体验 DLL 内的界面逻辑"));
    textLabel = new QLabel(this);
    textLabel->setObjectName("pluginTextLabel");
    layout->addWidget(textInput);
    layout->addWidget(textLabel);
    layout->addWidget(new QLabel(QStringLiteral("独立订阅设备状态消息"), this));
    deviceStatusLabel = new QLabel(this);
    deviceStatusLabel->setObjectName("contentTextDeviceStatus");
    layout->addWidget(deviceStatusLabel);
    layout->addStretch();
}

void GWidget::connectSignals()
{
    connect(textInput, &QLineEdit::textChanged, textLabel, &QLabel::setText);
}
