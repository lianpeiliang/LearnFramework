#include "CommandBindings.h"
#include "ServiceRegistry.h"
#include "PluginLoader.h"

CommandBindings::CommandBindings(WidgetFactory *factory)
{
    // 注册成功后，分发器通过引用计数管理这个堆对象；不能再手动 delete。
    auto *loader = new PluginLoader(factory);
    registered = ServiceRegistry::instance()->commandBus()->registerHandler(1000, loader) == 0;
    if (!registered)
        // 注册被拒绝时总线没有接管引用，申请者负责释放尚未使用的加载器。
        delete loader;
}

CommandBindings::~CommandBindings()
{
    // 最后一个命令绑定解除时，分发器调用 dispose，进而触发插件资源清理。
    if (registered)
        ServiceRegistry::instance()->commandBus()->unregisterHandler(1000);
}

