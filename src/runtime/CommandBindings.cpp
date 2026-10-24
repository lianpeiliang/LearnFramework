#include "CommandBindings.h"
#include "ServiceRegistry.h"
#include "PluginLoader.h"

CommandBindings::CommandBindings()
{
    // 注册成功后，分发器通过引用计数管理这个堆对象；不能再手动 delete。
    ServiceRegistry::instance()->commandBus()->registerHandler(
        1000, new PluginLoader);
}

CommandBindings::~CommandBindings()
{
    // 最后一个命令绑定解除时，分发器调用 dispose，进而触发插件资源清理。
    ServiceRegistry::instance()->commandBus()->unregisterHandler(1000);
}

