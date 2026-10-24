#pragma once

#include "CommandBus.h"

#include <QList>
#include <QSet>
#include <QString>
#include <QStringList>
#include <memory>

class WidgetFactory;
class PluginMessageHub;
struct LoadedPlugin;

/**
 * 插件加载的组织者，作为命令 1000 的处理器注册到 CommandBus。
 *
 * 主流程：扫描目录 → 加载 DLL 入口 → 注入可选通信服务 → initialize
 *         → 校验并注册界面名称 → 保存模块引用。
 *
 * Plugin 是必需接口；WidgetProvider 和 PluginServicesAware 是可选能力。
 * 本类不实现业务界面，只把插件提供的创建函数登记到 WidgetFactory。
 * 加载和界面创建在 GUI 线程同步执行；目前只驱动 initialize/shutdown，
 * 不调用 start/stop，也不提供运行中热替换 DLL 的能力。
 */
class PluginLoader : public CommandHandler
{
public:
    // factory 是借用指针，不由本类删除，须在加载操作期间有效。
    // 为 nullptr 时仍初始化插件，但不登记插件提供的 QWidget 类型。
    explicit PluginLoader(WidgetFactory *factory = nullptr);
    // 释放加载器持有的模块引用；工厂回调仍可能继续保持某个模块存活。
    ~PluginLoader() override;

    // 接收命令 1000；data 是 UTF-8 编码目录路径，length 是字节数。
    // data 只在本次同步调用中读取，不需要以 '\0' 结尾，也不会被保存。
    // 无效参数或任一候选插件加载失败返回 false，已成功加载的插件不回滚。
    bool dispatchCommand(unsigned int, char * = nullptr, int = 0) override;
    bool dispatchMessage(unsigned int, void * = nullptr, int = 0) override
    {
        // 满足处理器接口，但当前加载器没有消息处理职责。
        return false;
    }

private:
    // 只扫描目录这一层的库文件；目录不存在或没有候选库时返回 true。
    // 单个库失败不会中断扫描，最终返回所有候选库的汇总结果。
    bool loadDirectory(const QString &path);
    // 加载单个库。已成功加载的规范路径直接视为成功，避免重复初始化。
    // 只有初始化和类型注册均成功，才将模块加入 modules/loadedPaths。
    bool loadPlugin(const QString &path);
    // 获取 Plugin 入口，注入可选通信服务，再初始化并查询界面提供能力。
    // 失败后的资源清理由 LoadedPlugin 析构函数统一完成。
    bool initializePlugin(LoadedPlugin &module);
    // 将插件的“名称 → 创建函数”登记到工厂；注册过程不创建实际 QWidget。
    // 回调捕获模块共享引用，确保以后创建界面时插件仍然有效。
    bool registerWidgetTypes(const std::shared_ptr<LoadedPlugin> &module);
    // 检查空名称、插件内部重名、与现有工厂类型冲突；不修改工厂。
    // 仅在 factory 非空时调用，pluginPath 用于输出错误所在的 DLL 路径。
    bool validateWidgetTypes(const QStringList &names, const QString &pluginPath) const;

    // 外部窗口工厂；所有权属于 ApplicationHost 等调用方。
    WidgetFactory *factory;
    // 各插件通信作用域共享此消息中转对象，底层使用主程序的 CommandBus。
    std::shared_ptr<PluginMessageHub> messaging;
    // 本加载器持有的成功模块；工厂副本中的创建回调也持有模块共享引用。
    QList<std::shared_ptr<LoadedPlugin>> modules;
    // 只记录成功加载的规范路径，加载失败的文件可以在后续扫描时重试。
    QSet<QString> loadedPaths;
};
