# 08 对象所有权与退出清理

[返回目录](README.md) · [上一章](07-business-state.md) · [下一章](09-practice-and-review.md)

## 1. 为什么这一章很重要

插件窗口的析构函数、信号回调以及 std::function 内部的可调用对象，都可能使用 DLL 中的代码。若模块或共享状态先失效，窗口还在运行，之后一次点击或析构就可能访问无效对象。

因此，插件框架的正确性不只在于“能显示”，还在于“任何时刻都知道谁保证谁活着”。

## 2. 所有权速查表

| 对象/资源 | 主要拥有者 | 借用者或观察者 |
| --- | --- | --- |
| QApplication | main 的局部变量 | 所有 GUI 对象依赖其环境 |
| ApplicationHost | main 的局部变量 | 应用启动流程 |
| 根 QWidget | ApplicationHost 的 unique_ptr | 查找控件的代码 |
| 容器和内容 QWidget | Qt 父子关系 | 布局、测试、模块的 QPointer |
| PluginLoader | CommandBus 的命令绑定引用 | CommandBindings 负责注册/注销 |
| LoadedPlugin 记录 | 加载器和工厂回调的 shared_ptr | 内部接口指针 |
| 插件入口 QObject | QPluginLoader 管理 | Plugin*、WidgetProvider* |
| DeviceState / ReportState | 对应插件 | 该插件的面板 |
| PluginServicesScope | LoadedPlugin 的 unique_ptr | 插件、面板借用服务指针 |
| PluginMessageHub | 加载器和各服务作用域的 shared_ptr | 原始总线保存订阅地址 |
| 总线 | ServiceRegistry 的成员 | 宿主、加载器、hub |

注意 LoadedPlugin 是主程序里的资源记录，不等于 DLL 本身，也不等于插件入口对象。

## 3. 正常退出的第一层顺序

main 先构造 QApplication，再构造 core。离开 main 时逆序销毁：先 core，后 QApplication，确保清理窗口时 Qt 环境还存在。

ApplicationHost 析构函数体显式执行：

```cpp
window.reset();
widgetFactory.clear();
```

第一步销毁主窗口及其子控件；第二步释放原工厂回调。之后成员自动析构，CommandBindings 注销 1000；最后一份命令绑定消失时，总线 dispose 掉 PluginLoader。

成员的自动析构是逆序的，但析构函数体更早执行。不能只看成员排列，忽略这两行显式清理。

## 4. 模块最后一个引用消失时会发生什么

[LoadedPlugin 析构实现](C:/Users/35536/Desktop/LearnFramework/LearnFramework/src/runtime/PluginLoader.cpp) 的顺序是：

1. 删除记录中仍然存活的插件内容窗口。
2. 如果尝试过 initialize，调用 shutdown，释放插件业务状态。
3. 对通信插件调用 setPluginServices(nullptr)，撤回借用接口。
4. services.reset，清除命令和消息回调。
5. 调用 QPluginLoader::unload。

为什么不先清理服务？当前契约允许插件在 shutdown 期间仍然使用服务。为什么卸载在最后？回调对象自身的销毁也可能需要 DLL 代码。

shutdown 的 bool 返回值当前没有被加载器处理，清理流程不会根据它重试。插件应主动确保清理逻辑可靠。

## 5. 已经被父窗口删除的控件，会不会再删一次

模块用 QList<QPointer<QWidget>> 追踪窗口。Qt 删除窗口后，其 QPointer 自动变空。模块最终对这些指针调用 delete，空指针不会产生重复删除。

工厂 clear 本身只清除创建函数，不直接遍历全部窗口。但如果清除函数导致最后一个模块引用释放，模块析构会删除残留窗口。因此存在“清理工厂间接使窗口消失”的情况。

现有测试专门检查：加载器销毁后工厂副本仍可创建窗口；最后清空副本时，窗口先销毁，插件入口后销毁。

## 6. 工厂副本为什么既重要又需要克制

容器保存工厂副本，副本中的插件回调保留模块引用。只要这些函数仍可能被使用，就不能提前关闭模块。

这也意味着每个容器复制的是当时整个工厂的创建函数集合，可能保留多个插件的引用，不仅是当前显示内容对应的插件。

PluginLoader 析构会按加载逆序放弃自己的引用，但不能保证插件一定在那一刻按这个顺序关闭。其他工厂副本也能延迟最终释放。

## 7. Qt 父子关系并没有替代全部所有权管理

普通子标签和按钮由父 QWidget 回收；无 parent 的 DemoPlugin 工具窗口由插件 shutdown 主动删除；DeviceState 由设备插件主动删除；通信处理器由总线管理；模块记录由 shared_ptr 管理。

项目同时使用多种机制，每一种解决不同对象的归属。不要看到某个类继承 QObject，就推断它必然会被某个父对象自动回收。

DemoPlugin 的析构函数还会调用 shutdown，所以 shutdown 要允许重复调用。它用 QPointer 检查窗口是否仍在，删除后置空，适应重复清理。

## 8. 借用指针的使用边界

contentWidget 返回裸指针。setContentClass 替换内容之后，先前拿到的地址就可能失效。

PluginServices 是借用接口，插件不能 delete，也不能保存到插件生命周期之外。传给回调的 QVariantMap 引用只在本次同步调用期间有效，需要以后使用时应复制数据。

ServiceRegistry 的 windows/services 表同样只保存裸指针，不拥有对象，也没有自动移除悬空条目的完整机制。当前主流程主要使用其中的 commandBus；不要误以为所有业务状态都由注册表统一管理。

## 9. 一种实用的读码方法

看到一个成员指针时，逐一写出：

1. 谁创建？
2. 谁释放？
3. 谁会访问它？
4. 谁保证访问结束前它不会被释放？
5. 提前失败时谁收尾？

例如 DeviceMonitorWidget::state：DevicePlugin 创建和释放；监控借用；LoadedPlugin 在 shutdown 前先清理面板，所以监控不会在状态已释放后继续使用它。

本章自检：QPointer 能保证设备状态一定活着吗？不能。它只在 QObject 删除后置空；是否延长寿命是所有权机制的责任，而不是弱引用的能力。

