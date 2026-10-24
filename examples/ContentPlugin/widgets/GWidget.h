#pragma once

#include <QWidget>

class PluginServices;
class QLineEdit;
class QLabel;

// 文本输入面板，独立订阅设备状态。
class GWidget : public QWidget
{
public:
    GWidget(PluginServices *services, QWidget *parent);

private:
    void setupUi();
    void connectSignals();

    QLineEdit *textInput = nullptr;
    QLabel *textLabel = nullptr;
    QLabel *deviceStatusLabel = nullptr;
};
