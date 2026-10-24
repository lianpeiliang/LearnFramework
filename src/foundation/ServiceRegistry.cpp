#include "ServiceRegistry.h"

ServiceRegistry *ServiceRegistry::instance()
{
    // 函数局部静态对象只初始化一次，正常退出时自动析构。
    // 这只保证单例构造规则，不使窗口表、服务表或总线操作具备并发访问保护。
    static ServiceRegistry manager;
    return &manager;
}
