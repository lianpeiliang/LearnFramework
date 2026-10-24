#pragma once

#include <QtPlugin>

// Qt 插件的抽象契约：实现类还需通过 Q_INTERFACES 和 Q_PLUGIN_METADATA 声明接口。
// 返回 bool 表示该阶段是否成功；当前加载器只驱动 initialize/shutdown。
class Plugin
{
public:
    virtual ~Plugin();

    // 分配资源，完成初始化。
    virtual bool initialize() = 0;

    // 启动任务、定时器及消息订阅。
    virtual bool start() = 0;

    // 停止任务，撤销订阅，对应 start。
    virtual bool stop() = 0;

    // 释放资源，对应 initialize。
    virtual bool shutdown() = 0;
};

// 接口标识必须与插件实现匹配，Qt 才能通过 qobject_cast 获得 Plugin 接口。
Q_DECLARE_INTERFACE(Plugin, "org.learnframework.Plugin/1.0");
