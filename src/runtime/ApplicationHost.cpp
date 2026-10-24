#include "ApplicationHost.h"
#include "ServiceRegistry.h"
#include "DemoWidgets.h"
#include "RuntimeUiLoader.h"

#include <QCoreApplication>
#include <QDir>
#include <QDebug>
#include <QFile>
#include <QFileInfo>

ApplicationHost::ApplicationHost() : handler(&widgetFactory)
{
    registerDemoWidgets(widgetFactory);
}

ApplicationHost::~ApplicationHost()
{
    // 内容窗口和工厂回调先释放，然后才注销命令、关闭插件。
    window.reset();
    widgetFactory.clear();
}

ApplicationHost *ApplicationHost::instance()
{
    static ApplicationHost core;
    return &core;
}

bool ApplicationHost::show()
{
    if (!handler.isRegistered())
    {
        qCritical() << "Cannot start ApplicationHost: command 1000 is already registered";
        return false;
    }
    if (window)
    {
        window->show();
        return true;
    }

    const QString uiPath = QDir(QCoreApplication::applicationDirPath())
                               .filePath("ui/main.ui");
    QFile file(uiPath);
    if (!file.open(QIODevice::ReadOnly))
    {
        qCritical().noquote() << "Cannot open UI:" << uiPath << file.errorString();
        return false;
    }

    // 以可执行文件所在目录定位插件，避免受 Qt Creator 工作目录设置影响。
    QByteArray path = QDir(QCoreApplication::applicationDirPath())
                          .filePath("plugins")
                          .toUtf8();
    // 1000 是本学习版约定的“加载插件”命令，CommandBindings 将它绑定到 PluginLoader。
    // 分发为同步调用，path 在调用期间有效；处理器不能保存 data 指针供稍后使用。
    if (!ServiceRegistry::instance()->commandBus()->dispatchCommand(
            1000, path.data(), path.size()))
        qWarning() << "Some plugins failed to load; see previous messages";

    // 容器构造时会复制工厂，因此必须在创建容器之前完成插件注册。
    RuntimeUiLoader loader(widgetFactory);
    loader.setWorkingDirectory(QFileInfo(file).absoluteDir());
    window.reset(loader.load(&file));
    if (!window)
    {
        qCritical().noquote() << "Cannot load UI:" << uiPath << loader.errorString();
        return false;
    }

    // 插件先注册类型，再按 UI 属性创建容器内容。
    QString error;
    if (!loader.populateContainers(window.get(), &error))
    {
        qCritical().noquote() << "Cannot populate UI:" << uiPath << error;
        window.reset();
        return false;
    }

    window->show();
    return true;
}
