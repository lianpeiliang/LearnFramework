# 02 读代码必备的 C++ 与 Qt 概念

[返回目录](README.md) · [上一章](01-overview.md) · [下一章](03-startup.md)

## 1. .h、.cpp、.pro 分别是什么

`.h` 主要描述“有什么类、有什么成员、能调用什么函数”；`.cpp` 主要描述“这些函数具体如何做”；`.pro` 是 qmake 工程配置，告诉构建系统编译哪些源文件、使用哪些 Qt 模块、生成应用还是库。

例如 WidgetFactory.h 声明 create，WidgetFactory.cpp 实现查表和调用创建函数。阅读时先看头文件获得职责，再看实现中的控制流程。

`#include` 让编译器看到声明，并不等于运行时加载 DLL。运行时加载是 QPluginLoader 做的另一件事。

## 2. 类、对象、指针和所有权

`CWidget` 是类型，`new CWidget(parent)` 创建一个对象。`QWidget *` 是指向对象的地址，调用 `widget->show()` 是让这个地址对应的对象显示。

拿着地址不一定拥有对象。项目里最常见的区别是：

```cpp
PluginServices *services = nullptr; // 借用，不负责 delete
std::unique_ptr<QWidget> window;    // 拥有，负责释放
```

“借用”表示别人保证对象活着，你只临时使用。看到裸指针时必须继续问：谁创建它？谁删除它？借用者会不会活得比拥有者久？

`nullptr` 表示没有对象。判空只能判断当前指针值是否为空，普通裸指针不会在对象被别人删除后自动变成空值。

## 3. 构造与析构

`ApplicationHost core;` 创建栈对象，离开作用域时自动调用析构函数。`new` 创建的对象需要通过明确的所有权机制回收。

C++ 成员按**头文件中的声明顺序**构造，按相反顺序析构，析构函数体先于成员的自动析构执行。这解释了 ApplicationHost 为什么把 widgetFactory 声明在 handler 前面：handler 构造时要借用已经存在的工厂。

这种“对象创建时取得资源、对象销毁时归还资源”的方式叫 RAII。CommandBindings 就是一个例子：构造注册命令，析构注销命令。

## 4. 接口、继承和虚函数

源码片段：

```cpp
virtual bool initialize() = 0;
```

`virtual` 允许通过基类接口调用具体实现；`= 0` 表示纯虚函数，要求具体插件实现。`override` 让编译器检查派生类是否真的覆盖了接口函数。

主程序持有 `Plugin *`，调用 `plugin->initialize()`，实际执行的可以是 DevicePlugin::initialize。主程序只需要公共契约，不需要把 DevicePlugin.cpp 编译进自己。

虚析构保证通过基类删除时能正确执行派生类析构。但项目中的 Qt 插件入口由 QPluginLoader 管理，不能因为接口有虚析构，就随意 `delete plugin`。

## 5. Lambda 和 std::function

源码片段：

```cpp
factory.registerWidget("c", [](QWidget *parent) {
    return new CWidget(parent);
});
```

读法：登记名称 c，并保存一段“以后收到 parent 时创建 CWidget”的小函数。方括号是捕获列表，小括号是参数，花括号是函数体。此时没有创建 CWidget；以后工厂执行该函数才创建。

`std::function<QWidget *(QWidget *)>` 是能保存这类可调用对象的变量类型。项目用别名 `WidgetFactory::Creator` 让它更易读。

插件回调写成 `[module, name]`，表示把共享指针和名称按值保存在函数对象中；即使循环结束，回调仍有自己的名称和模块引用。

`[this]` 只保存当前对象的地址，不会自动延长当前对象的寿命。Qt connect 的上下文对象就是防止某些回调在对象销毁后继续执行的重要措施。

## 6. QWidget、布局和 QObject 父子关系

QWidget 是窗口/控件的基础类型，按钮、输入框以及业务面板都属于 QWidget 体系。QVBoxLayout 把控件纵向排列，QGridLayout 按行列分配位置。

`new QLabel(this)` 的 this 是父对象。父对象销毁时，Qt 会销毁子对象，因此不需要在面板析构中逐个删除这些标签。

布局决定位置，父子关系决定对象归属。两者经常一起建立，但理解时不要混为一谈。

## 7. 信号槽和事件循环

源码片段：

```cpp
connect(input, &QLineEdit::textChanged, label, &QLabel::setText);
```

输入框发出 textChanged 信号时，标签执行 setText。这就是信号与接收函数的连接。

另一种形式是：

```cpp
connect(button, &QPushButton::clicked, this, [this] {
    requestDeviceConnection(true);
});
```

这里第三个参数 this 是连接上下文，对象销毁时连接会自动断开。

`app.exec()` 进入 Qt 事件循环，持续处理鼠标、键盘、窗口等事件。项目自己的 CommandBus 则是普通的同步函数调用：执行完处理器才返回。不能看到“消息”二字，就认为它会自动排队或切换线程。

## 8. 三种容易混淆的指针工具

| 工具 | 负责延长寿命吗 | 本项目例子 |
| --- | --- | --- |
| unique_ptr | 是，一个独占拥有者 | ApplicationHost 的主窗口 |
| shared_ptr | 是，最后一个共享引用释放时销毁 | LoadedPlugin、PluginMessageHub |
| QPointer | 否，QObject 被销毁后自动置空 | 已创建窗口的追踪记录 |

CommandBus 自己维护的引用计数又是另一套机制，它数的是“处理器绑定了多少条命令”，并非全程序有多少指针指向该对象。

## 9. 名称与 Qt 元对象机制

`objectName` 是对象实例的查找名，例如 contentCounterContainer。`contentClass` 是动态属性，内容为工厂配置键，例如 plugin.f。二者职责不同。

`Q_OBJECT` 为类接入 Qt 元对象能力；qmake 会安排 moc 生成相关辅助代码。`Q_PLUGIN_METADATA` 描述插件入口元数据；`Q_INTERFACES` 声明插件支持的 Qt 接口；`Q_DECLARE_INTERFACE` 为公共接口绑定 IID 字符串。主程序再用 `qobject_cast` 查询接口。

初学时先记住：普通 C++ 继承关系加上这些 Qt 声明，才能完成这里的插件能力查询。

## 10. 本章自检

为什么保存 `[module, name]` 比保存对循环局部变量的引用更合适？因为创建窗口发生在以后，局部变量可能早已离开作用域；按值捕获保留了回调需要的信息，shared_ptr 还保留模块寿命。

源码入口：[内置控件](C:/Users/35536/Desktop/LearnFramework/LearnFramework/src/widgets/DemoWidgets.cpp)、[插件公共接口](C:/Users/35536/Desktop/LearnFramework/LearnFramework/src/foundation/Plugin.h)、[窗口工厂声明](C:/Users/35536/Desktop/LearnFramework/LearnFramework/src/widgets/WidgetFactory.h)。

