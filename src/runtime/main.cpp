#include <QApplication>
#include "ApplicationHost.h"

int main(int argc, char *argv[])
{
    // Qt GUI 的基础环境：窗口、事件派发等都要求先创建 QApplication。
    QApplication app(argc, argv);
    // 栈对象按创建顺序的逆序销毁：核心窗口和插件先释放，Qt 环境后释放。
    // 本入口不使用 ApplicationHost::instance()，避免静态窗口晚于 Qt 环境销毁。
    ApplicationHost core;
    if (!core.show())
        return 1;
    // 进入事件循环，等待鼠标、键盘、窗口关闭等事件；退出后才执行局部对象析构。
    return app.exec();
}
