#pragma once

#include "CommandBus.h"

#include <QList>
#include <QSet>
#include <QString>
#include <memory>

class WidgetFactory;
class PluginMessageHub;
struct LoadedPlugin;

// 把“加载插件”命令转为 Qt 动态库加载操作，不处理业务数据。
class PluginLoader : public CommandHandler
{
public:
    explicit PluginLoader(WidgetFactory *factory = nullptr);
    ~PluginLoader() override;

    // 接收命令 1000；data 是 UTF-8 编码目录路径，length 是字节数。
    bool dispatchCommand(unsigned int, char * = nullptr, int = 0) override;
    bool dispatchMessage(unsigned int, void * = nullptr, int = 0) override
    {
        // 满足处理器接口，但当前加载器没有消息处理职责。
        return false;
    }

private:
    WidgetFactory *factory;
    std::shared_ptr<PluginMessageHub> messaging;
    QList<std::shared_ptr<LoadedPlugin>> modules;
    QSet<QString> loadedPaths;
};
