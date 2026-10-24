#include "PluginLoader.h"
#include "Plugin.h"

#include <QDebug>
#include <QDir>
#include <QLibrary>
#include <QPluginLoader>

PluginLoader::~PluginLoader()
{
    // 插件初始化可能有先后依赖，因此按逆序释放插件自己的资源。
    // 必须先 shutdown，再删除加载器；shutdown 执行时仍需要库中的代码。
    for (int i = plugins.size() - 1; i >= 0; --i)
        plugins.at(i)->shutdown();

    // 删除加载器对象；动态库是否实际卸载还受 Qt 的加载提示与其他引用影响。
    qDeleteAll(loaders);
}

bool PluginLoader::dispatchCommand(unsigned int id, char *data, int length)
{
    // 拒绝不支持的命令和无效载荷，避免把任意字节当成路径。
    if (id != 1000 || !data || length <= 0)
        return false;

    QDir directory(QString::fromUtf8(data, length));
    bool success = true;

    // 仅扫描这一层目录；目录不存在或没有插件时，当前实现返回 true。
    for (const QString &name : directory.entryList(QDir::Files))
    {
        QString path = directory.absoluteFilePath(name);
        // 由 Qt 按平台识别库文件名，兼容 Windows DLL 和其他平台的库后缀。
        if (!QLibrary::isLibrary(path))
            continue;

        QPluginLoader *loader = new QPluginLoader(path);
        // instance 创建/获取 Qt 插件对象，qobject_cast 检查是否实现约定的 Plugin 接口。
        Plugin *plugin = qobject_cast<Plugin *>(loader->instance());
        if (!plugin)
        {
            qWarning() << path << loader->errorString();
            delete loader;
            success = false;
            continue;
        }

        // 初始化失败也调用 shutdown，以回收插件可能已分配的部分资源。
        if (!plugin->initialize())
        {
            plugin->shutdown();
            delete loader;
            success = false;
            continue;
        }

        // 只记录初始化成功的插件；本阶段不调用 start/stop。
        loaders.append(loader);
        plugins.append(plugin);
    }

    // 任意一个候选库加载失败都会返回 false，但不回滚已成功初始化的插件。
    return success;
}
