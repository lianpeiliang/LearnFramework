#pragma once

#include <QWidget>

class PluginServices;
class QLineEdit;
class QLabel;

// 文本输入面板，独立订阅设备状态。
// 文本仅属于当前窗口；与 F 面板共享的是设备消息来源，而非输入内容。
class GWidget : public QWidget
{
public:
    GWidget(PluginServices *services, QWidget *parent);

private:
    void setupUi();
    void connectSignals();

    // 控件均由本 QWidget 作为父对象拥有，这里保留指针用于建立界面连接。
    QLineEdit *textInput = nullptr;
    QLabel *textLabel = nullptr;
    QLabel *deviceStatusLabel = nullptr;
};
