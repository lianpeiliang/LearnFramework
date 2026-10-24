#include "ApplicationHost.h"
#include "ServiceRegistry.h"
#include "DemoWidgets.h"
#include "RuntimeUiLoader.h"

#include <QCoreApplication>
#include <QDir>
#include <QDebug>
#include <QFile>
#include <QFileInfo>

ApplicationHost::ApplicationHost()
{
    registerDemoWidgets(widgetFactory);
}

ApplicationHost *ApplicationHost::instance()
{
    static ApplicationHost core;
    return &core;
}

bool ApplicationHost::show()
{
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

    RuntimeUiLoader loader(widgetFactory);
    // UI 中的相对图片路径以 UI 所在目录为基准。
    loader.setWorkingDirectory(QFileInfo(file).absoluteDir());
    window.reset(loader.load(&file));
    if (!window)
    {
        qCritical().noquote() << "Cannot load UI:" << uiPath << loader.errorString();
        return false;
    }

    QString error;
    if (!loader.populateContainers(window.get(), &error))
    {
        qCritical().noquote() << "Cannot populate UI:" << uiPath << error;
        window.reset();
        return false;
    }

    // 以可执行文件所在目录定位插件，避免受 Qt Creator 工作目录设置影响。
    QByteArray path = QDir(QCoreApplication::applicationDirPath())
                          .filePath("plugins")
                          .toUtf8();
    // 1000 是本学习版约定的“加载插件”命令，CommandBindings 将它绑定到 PluginLoader。
    // 分发为同步调用，path 在调用期间有效；处理器不能保存 data 指针供稍后使用。
    ServiceRegistry::instance()->commandBus()->dispatchCommand(
        1000, path.data(), path.size());

    window->show();
    return true;
}
