#pragma once

#include "CommandBindings.h"

#include <QWidget>
#include <memory>

// 应用组装层：拥有主窗口，并通过 CommandBindings 安装核心命令处理器。
class ApplicationHost
{
public:
    // 备用的静态实例入口；当前 main 使用局部对象来明确控制 GUI 对象生命周期。
    static ApplicationHost *instance();
    bool show();

private:
    // 窗口类型、属性和布局由外部 ui/main.ui 决定。
    std::unique_ptr<QWidget> window;
    // 构造 ApplicationHost 时自动注册命令，析构时自动注销。
    CommandBindings handler;
};
