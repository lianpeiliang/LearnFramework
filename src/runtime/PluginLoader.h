#pragma once

#include "CommandBus.h"

#include <QList>

class QPluginLoader;
class Plugin;

// 把“加载插件”命令转为 Qt 动态库加载操作，不处理业务数据。
class PluginLoader : public CommandHandler
{
public:
    ~PluginLoader() override;

    // 接收命令 1000；data 是 UTF-8 编码目录路径，length 是字节数。
    bool dispatchCommand(unsigned int, char * = nullptr, int = 0) override;
    bool dispatchMessage(unsigned int, void * = nullptr, int = 0) override
    {
        // 满足处理器接口，但当前加载器没有消息处理职责。
        return false;
    }

private:
    QList<QPluginLoader *> loaders; // 本类持有并删除加载器，保持插件库的加载期。
    QList<Plugin *> plugins;        // 借用 Qt 插件实例，不能手动 delete。
};
