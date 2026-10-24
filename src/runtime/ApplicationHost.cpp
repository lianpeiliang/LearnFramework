#include "ApplicationHost.h"
#include "ServiceRegistry.h"

#include <QCoreApplication>
#include <QDir>

ApplicationHost *ApplicationHost::instance()
{
    static ApplicationHost core;
    return &core;
}

void ApplicationHost::show()
{
    // 以可执行文件所在目录定位插件，避免受 Qt Creator 工作目录设置影响。
    QByteArray path = QDir(QCoreApplication::applicationDirPath())
                          .filePath("plugins")
                          .toUtf8();
    // 1000 是本学习版约定的“加载插件”命令，CommandBindings 将它绑定到 PluginLoader。
    // 分发为同步调用，path 在调用期间有效；处理器不能保存 data 指针供稍后使用。
    ServiceRegistry::instance()->commandBus()->dispatchCommand(
        1000, path.data(), path.size());

    // 只显示空主窗口，框架本身不加入具体业务界面。
    window.setWindowTitle("Framework Learning");
    window.resize(800, 600);
    window.show();
}
