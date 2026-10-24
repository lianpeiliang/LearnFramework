#include "Plugin.h"


Plugin::~Plugin()
{
    // 虚析构保证通过接口删除时执行派生类析构；资源释放仍应按生命周期调用 shutdown。

}

