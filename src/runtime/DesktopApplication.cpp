#include "DesktopApplication.h"
#include "ApplicationHost.h"
#include <QTimer>

DesktopApplication::DesktopApplication(int &argc, char **argv)
    : QApplication(argc, argv)
{
    if (!ApplicationHost::instance()->show())
        // 构造时事件循环尚未启动，排队到事件循环开始后再设置失败退出码。
        QTimer::singleShot(0, this, [] { QCoreApplication::exit(1); });
}
