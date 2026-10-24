#pragma once

#include <QApplication>

// 备用的启动封装，当前 main 未使用；学习入口以 main.cpp 为准。
// 它使用静态 ApplicationHost，使用前需另行处理窗口早于 QApplication 销毁的顺序。
class DesktopApplication : public QApplication
{
public:
    DesktopApplication(int &argc, char **argv);
};
