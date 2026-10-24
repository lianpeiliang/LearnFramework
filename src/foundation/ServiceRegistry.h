#pragma once

#include "CommandBus.h"

#include <QMap>
#include <QString>
#include <QWidget>

// 注册表只记录对象，不承担窗口和共享服务的所有权。
// 窗口/服务销毁后应避免继续查找和使用其指针，当前版本不自动移除悬空条目。
class ServiceRegistry
{
public:
    // 返回共享注册表；当前用于 GUI 线程，不提供并发访问保护。
    static ServiceRegistry *instance();

    CommandBus *commandBus()
    {
        // 返回成员地址，调用者借用它，不应 delete。
        return &dispatcher;
    }

    bool registerWindow(const char *name, QWidget *widget)
    {
        // 同名注册会覆盖旧条目，不会销毁旧窗口；name 须为有效的 UTF-8 字符串。
        windows.insert(QString::fromUtf8(name), widget);
        return true;
    }

    QWidget *findWindow(const char *name)
    {
        // 未注册时返回 nullptr，调用者需要判空。
        return windows.value(QString::fromUtf8(name), nullptr);
    }

    bool registerService(int id, QObject *object)
    {
        // 服务 ID 是调用方约定的索引；这里不依赖任何具体服务类。
        services.insert(id, object);
        return true;
    }

    QObject *findService(int id)
    {
        return services.value(id, nullptr);
    }

private:
    CommandBus dispatcher;
    QMap<QString, QWidget *> windows;
    QMap<int, QObject *> services;
};
