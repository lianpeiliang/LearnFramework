#include "GWidget.h"
#include "DeviceStatusBinding.h"

#include <QLabel>
#include <QLineEdit>
#include <QVBoxLayout>

GWidget::GWidget(PluginServices *services, QWidget *parent) : QWidget(parent)
{
    setupUi();
    connectSignals();
    // 每个 G 面板建立自己的订阅，关闭当前面板不影响 F 或其他 G 面板。
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
    // 输入直接映射到本窗口标签，不经过跨插件总线，也不会改动设备状态。
    connect(textInput, &QLineEdit::textChanged, textLabel, &QLabel::setText);
}
