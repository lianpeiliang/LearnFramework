#pragma once

class PluginServices;
class QWidget;
class QLabel;

// 两个面板共用：订阅状态消息、更新标签，并请求初始状态。
// owner 销毁时，主程序自动取消订阅。
void observeDevice(PluginServices *services, QWidget *owner, QLabel *status);
