#pragma once

#include "CommandBindings.h"
#include "WidgetFactory.h"

#include <QWidget>
#include <memory>

// 应用组装层：拥有主窗口，并通过 CommandBindings 安装核心命令处理器。
class ApplicationHost
{
public:
    ApplicationHost();
    ~ApplicationHost();
    // 首次调用完成插件加载、外部 UI 解析和内容创建；后续调用显示已有窗口。
    // 返回 false 表示核心命令冲突、UI 无法读取/解析，或某个容器无法创建内容。
    bool show();

private:
    // 成员按声明顺序构造：工厂先就绪，命令绑定才能借用其地址。
    // 析构函数体先释放窗口与工厂回调，再由 handler 的析构函数注销加载器。
    WidgetFactory widgetFactory;
    CommandBindings handler;
    // 窗口类型、属性和布局由外部 ui/main.ui 决定。
    std::unique_ptr<QWidget> window;
};
