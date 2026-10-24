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

// 单个 DLL 的资源与生命周期记录，仅供本实现文件使用。
// PluginLoader 和工厂回调共同持有它；最后一个 shared_ptr 释放时自动清理。
struct LoadedPlugin
{
    explicit LoadedPlugin(const QString &path) : loader(path) {}

    // 工厂回调实际创建界面的入口；注册时已确认 provider 非空。
    QWidget *createWidget(const QString &name, QWidget *parent)
    {
        QWidget *widget = provider->createWidget(name, parent);
        if (!widget)
            return nullptr;

        // 统一父子关系，让容器拥有内容窗口，即使插件遗漏设置 parent。
        if (widget->parentWidget() != parent)
            widget->setParent(parent);
        // QPointer 在窗口被删除后自动变空，移除空记录以免频繁切换时积累。
        for (int i = widgets.size() - 1; i >= 0; --i)
            if (widgets.at(i).isNull())
                widgets.removeAt(i);
        widgets.append(widget);
        return widget;
    }

    ~LoadedPlugin()
    {
        // 清理顺序：内容窗口 → 插件业务资源 → 通信回调 → Qt 插件加载引用。
        // 窗口的析构代码和回调都可能位于 DLL 中，因此不能提前释放插件。
        for (int i = widgets.size() - 1; i >= 0; --i)
            delete widgets.at(i).data();
        // initialize 即使返回 false，也可能已分配部分资源，需要 shutdown 收尾。
        if (initializationAttempted)
            plugin->shutdown();
        // shutdown 期间仍可使用服务，完成后撤回插件借用的服务指针。
        if (servicesAware)
            servicesAware->setPluginServices(nullptr);
        // std::function 可能包含 DLL 内的析构代码，必须在卸载前移除全部回调。
        services.reset();
        // 保留 Qt 默认 PreventUnloadHint，不承诺运行中物理卸载 DLL。
        loader.unload();
    }

    QPluginLoader loader; // 管理 Qt 插件入口及动态库加载引用。
    Plugin *plugin = nullptr; // 必需的生命周期接口，借用 Qt 管理的入口对象。
    WidgetProvider *provider = nullptr; // 可选的 QWidget 创建接口，不能手动 delete。
    PluginServicesAware *servicesAware = nullptr; // 可选的通信服务注入接口。
    std::unique_ptr<PluginServicesScope> services; // 本模块独占的通信注册作用域。
    bool initializationAttempted = false; // 区分“尚未初始化”和“已尝试但可能失败”。
    QList<QPointer<QWidget>> widgets; // 窗口的弱引用，用于清理尚未被父对象释放的窗口。
};

// 消息中转对象由加载器创建，各模块的服务作用域持有它的共享引用。
// 这里借用的是主程序已有的总线，不在 DLL 中创建另一份 ServiceRegistry。
PluginLoader::PluginLoader(WidgetFactory *factory)
    : factory(factory), messaging(std::make_shared<PluginMessageHub>(ServiceRegistry::instance()->commandBus())) {}

PluginLoader::~PluginLoader()
{
    // 按加载的逆序放弃引用。若工厂回调仍持有模块，实际关闭时间会延后，
    // 并不保证所有模块都在此时立即 shutdown；这是避免回调悬空所必需的。
    while (!modules.isEmpty())
        modules.takeLast().reset();
}

bool PluginLoader::dispatchCommand(unsigned int id, char *data, int length)
{
    // 命令入口只验证载荷并转换编码，具体加载步骤交给下层函数。
    if (id != 1000 || !data || length <= 0)
        return false;
    return loadDirectory(QString::fromUtf8(data, length));
}

bool PluginLoader::loadDirectory(const QString &path)
{
    QDir directory(path);
    bool success = true;
    for (const QString &name : directory.entryList(QDir::Files, QDir::Name))
    {
        const QString pluginPath = directory.absoluteFilePath(name);
        // 先按平台识别库文件后缀；是否为有效 Qt 插件留给 QPluginLoader 检查。
        if (!QLibrary::isLibrary(pluginPath))
            continue;
        // 一个插件失败后继续扫描其他插件，最后汇总结果。
        if (!loadPlugin(pluginPath))
            success = false;
    }
    return success;
}

bool PluginLoader::loadPlugin(const QString &path)
{
    // 使用规范路径去重，避免同一文件因路径写法不同而被重复初始化。
    const QString canonicalPath = QFileInfo(path).canonicalFilePath();
    if (loadedPaths.contains(canonicalPath))
        return true;

    auto module = std::make_shared<LoadedPlugin>(path);
    // 失败时局部共享指针释放模块，自动关闭插件并回收已注册的通信回调。
    if (!initializePlugin(*module) || !registerWidgetTypes(module))
        return false;

    // 到这里才提交成功记录；失败的模块不会进入去重集合。
    modules.append(module);
    loadedPaths.insert(canonicalPath);
    return true;
}

bool PluginLoader::initializePlugin(LoadedPlugin &module)
{
    // instance() 负责加载库并创建/取得插件入口对象，还没有创建业务 QWidget。
    QObject *instance = module.loader.instance();
    // 通过 Qt 的接口声明查询能力，插件必须实现 Plugin 才能被接受。
    module.plugin = qobject_cast<Plugin *>(instance);
    if (!module.plugin)
    {
        qWarning() << "Cannot load Plugin:" << module.loader.fileName() << module.loader.errorString();
        return false;
    }

    // 可选通信能力在 initialize 前注入，旧插件可以不实现此接口。
    module.servicesAware = qobject_cast<PluginServicesAware *>(instance);
    if (module.servicesAware)
    {
        module.services.reset(new PluginServicesScope(messaging));
        module.servicesAware->setPluginServices(module.services.get());
    }
    // 必须在调用前标记，确保初始化中途失败也会进入 shutdown 清理。
    module.initializationAttempted = true;
    if (!module.plugin->initialize())
    {
        qWarning() << "Plugin initialization failed:" << module.loader.fileName();
        return false;
    }
    // 普通业务插件可以不提供界面，例如只处理后台业务的 Plugin 实现。
    module.provider = qobject_cast<WidgetProvider *>(instance);
    return true;
}

bool PluginLoader::registerWidgetTypes(const std::shared_ptr<LoadedPlugin> &module)
{
    // 无界面能力或未传入工厂都不是加载错误，插件的初始化仍然保留。
    if (!module->provider || !factory)
        return true;

    const QStringList names = module->provider->widgetClasses();
    // 先完整校验，再统一注册，避免名称冲突导致部分注册成功。
    if (!validateWidgetTypes(names, module->loader.fileName()))
        return false;

    for (const QString &name : names)
    {
        // 第一步：声明一个变量，用来保存“接收 parent、返回 QWidget*”的函数。
        WidgetFactory::Creator creator;

        // 第二步：为变量赋予具体的创建方法。这里保存方法，不会立即创建窗口。
        // module 按值复制共享指针，让模块保持有效；name 按值保存本次循环的名称。
        creator = [module, name](QWidget *parent) -> QWidget * {
            // 以后工厂调用 creator(parent) 时才执行此处，parent 由容器传入。
            QWidget *widget = module->createWidget(name, parent);
            return widget;
        };

        // 第三步：登记“名称 → 创建方法”，以后可按 name 查找并调用该方法。
        factory->registerWidget(name, creator);
    }
    return true;
}

bool PluginLoader::validateWidgetTypes(const QStringList &names, const QString &pluginPath) const
{
    // seen 检查本插件内部重名，factory 检查与内置类及其他插件的冲突。
    QSet<QString> seen;
    for (const QString &name : names)
    {
        if (name.trimmed().isEmpty() || seen.contains(name) || factory->contains(name))
        {
            qWarning() << "Conflicting or empty widget class:" << name << "in" << pluginPath;
            return false;
        }
        seen.insert(name);
    }
    return true;
}
