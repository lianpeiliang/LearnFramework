#include "PluginLoader.h"
#include "Plugin.h"
#include "PluginMessaging.h"
#include "ServiceRegistry.h"
#include "WidgetFactory.h"
#include "WidgetProvider.h"

#include <QDebug>
#include <QDir>
#include <QFileInfo>
#include <QLibrary>
#include <QPluginLoader>
#include <QPointer>
#include <QWidget>

struct LoadedPlugin
{
    explicit LoadedPlugin(const QString &path) : loader(path) {}

    ~LoadedPlugin()
    {
        // 工厂副本都释放后才会到这里；残留内容也必须先于 shutdown 释放。
        for (int i = widgets.size() - 1; i >= 0; --i)
            delete widgets.at(i).data();
        if (initializationAttempted)
            plugin->shutdown();
        if (servicesAware)
            servicesAware->setPluginServices(nullptr);
        // std::function 可能包含 DLL 内的析构代码，必须在卸载前移除全部回调。
        services.reset();
        // 保留 Qt 默认 PreventUnloadHint，不承诺运行中物理卸载 DLL。
        loader.unload();
    }

    QPluginLoader loader;
    Plugin *plugin = nullptr;
    WidgetProvider *provider = nullptr;
    PluginServicesAware *servicesAware = nullptr;
    std::unique_ptr<PluginServicesScope> services;
    bool initializationAttempted = false;
    QList<QPointer<QWidget>> widgets;
};

PluginLoader::PluginLoader(WidgetFactory *factory)
    : factory(factory), messaging(std::make_shared<PluginMessageHub>(ServiceRegistry::instance()->commandBus())) {}

PluginLoader::~PluginLoader()
{
    // 工厂回调中的共享引用可继续保持插件有效，避免复制工厂后留下悬空指针。
    while (!modules.isEmpty())
        modules.takeLast().reset();
}

bool PluginLoader::dispatchCommand(unsigned int id, char *data, int length)
{
    if (id != 1000 || !data || length <= 0)
        return false;

    QDir directory(QString::fromUtf8(data, length));
    bool success = true;
    for (const QString &name : directory.entryList(QDir::Files, QDir::Name))
    {
        const QString path = directory.absoluteFilePath(name);
        if (!QLibrary::isLibrary(path))
            continue;
        const QString canonicalPath = QFileInfo(path).canonicalFilePath();
        if (loadedPaths.contains(canonicalPath))
            continue;

        auto module = std::make_shared<LoadedPlugin>(path);
        QObject *instance = module->loader.instance();
        module->plugin = qobject_cast<Plugin *>(instance);
        if (!module->plugin)
        {
            qWarning() << "Cannot load Plugin:" << path << module->loader.errorString();
            success = false;
            continue;
        }
        module->servicesAware = qobject_cast<PluginServicesAware *>(instance);
        if (module->servicesAware)
        {
            module->services.reset(new PluginServicesScope(messaging));
            module->servicesAware->setPluginServices(module->services.get());
        }
        module->initializationAttempted = true;
        if (!module->plugin->initialize())
        {
            qWarning() << "Plugin initialization failed:" << path;
            success = false;
            continue;
        }

        module->provider = qobject_cast<WidgetProvider *>(instance);
        const QStringList names = module->provider && factory
                                      ? module->provider->widgetClasses() : QStringList();
        QSet<QString> seen;
        bool valid = true;
        for (const QString &key : names)
        {
            if (key.trimmed().isEmpty() || seen.contains(key) || factory->contains(key))
            {
                qWarning() << "Conflicting or empty widget class:" << key << "in" << path;
                valid = false;
                break;
            }
            seen.insert(key);
        }
        if (!valid)
        {
            // 整个插件不注册，避免部分注册成功的状态。
            success = false;
            continue;
        }

        for (const QString &key : names)
        {
            factory->registerWidget(key, [module, key](QWidget *parent) -> QWidget * {
                QWidget *widget = module->provider->createWidget(key, parent);
                if (widget)
                {
                    if (widget->parentWidget() != parent)
                        widget->setParent(parent);
                    for (int i = module->widgets.size() - 1; i >= 0; --i)
                        if (module->widgets.at(i).isNull())
                            module->widgets.removeAt(i);
                    module->widgets.append(widget);
                }
                return widget;
            });
        }
        modules.append(module);
        loadedPaths.insert(canonicalPath);
    }
    return success;
}
