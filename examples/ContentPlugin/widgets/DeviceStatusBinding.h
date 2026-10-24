#pragma once

class PluginServices;
class QWidget;
class QLabel;

// 两个面板共用：订阅状态消息、更新标签，并请求初始状态。
// owner 应拥有 status；owner 销毁时，主程序自动断开回调，避免访问已释放的标签。
void observeDevice(PluginServices *services, QWidget *owner, QLabel *status);
