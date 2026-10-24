#include "Plugin.h"


Plugin::~Plugin()
{
    // 虚析构保证通过接口删除时执行派生类析构；这里不自动调用生命周期虚函数。
    // Qt 插件入口对象由 QPluginLoader 管理，加载器负责先调用 shutdown 释放业务资源。

}

