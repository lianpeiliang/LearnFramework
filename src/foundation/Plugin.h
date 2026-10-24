#pragma once

#include <QtPlugin>

// Qt 插件的抽象契约：实现类还需通过 Q_INTERFACES 和 Q_PLUGIN_METADATA 声明接口。
// 返回 bool 表示该阶段是否成功；当前加载器只驱动 initialize/shutdown。
class Plugin
{
public:
    virtual ~Plugin();

    // 分配资源并建立服务；当前加载器在本阶段之前注入可选的 PluginServices。
    virtual bool initialize() = 0;

    // 预留的运行阶段：启动任务、定时器及消息订阅；当前加载器不会自动调用。
    virtual bool start() = 0;

    // 预留的停止阶段：撤销 start 建立的活动；当前加载器不会自动调用。
    virtual bool stop() = 0;

    // 释放资源，对应 initialize；初始化返回 false 后也会调用，需能清理部分初始化状态。
    virtual bool shutdown() = 0;
};

// 接口标识必须与插件实现匹配，Qt 才能通过 qobject_cast 获得 Plugin 接口。
Q_DECLARE_INTERFACE(Plugin, "org.learnframework.Plugin/1.0");
