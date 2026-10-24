#include "ServiceRegistry.h"

ServiceRegistry *ServiceRegistry::instance()
{
    // 函数局部静态对象只初始化一次，进程结束时自动析构。
    static ServiceRegistry manager;
    return &manager;
}
