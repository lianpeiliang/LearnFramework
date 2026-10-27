# 03 从 main 到主窗口

[返回目录](README.md) · [上一章](02-basics.md) · [下一章](04-ui-and-factory.md)

## 1. 程序入口只有几行，但背后有两条阶段线

[main.cpp](C:/Users/35536/Desktop/LearnFramework/LearnFramework/src/runtime/main.cpp) 的核心源码：

```cpp
QApplication app(argc, argv);
ApplicationHost core;
if (!core.show())
    return 1;
return app.exec();
```

先建立 Qt GUI 环境，再组装应用并显示，最后进入事件循环。如果 show 失败，直接返回 1，不进入事件循环；局部对象仍然会正常析构。

注意 `ApplicationHost::show()` 不只是 QWidget 的 show。它是项目自己定义的函数，包含加载插件、读 UI 和填充容器等初始化步骤。

## 2. 第一步：构造 ApplicationHost

[头文件](C:/Users/35536/Desktop/LearnFramework/LearnFramework/src/runtime/ApplicationHost.h) 的成员按顺序为：

```cpp
WidgetFactory widgetFactory;
CommandBindings handler;
std::unique_ptr<QWidget> window;
```

在构造函数体执行前：

1. 工厂先构造完成，此时没有登记业务内容。
2. handler 收到工厂地址，创建 PluginLoader。
3. PluginLoader 创建 PluginMessageHub，并借用 ServiceRegistry 中的总线。
4. CommandBindings 把编号 1000 绑定到 PluginLoader。
5. window 初始为空。
6. 最后进入 ApplicationHost 构造函数体，登记内置的 c、d、e。

所以“创建加载器”早于“登记内置类”，但**真正扫描插件**发生在后续 show 中，此时内置类已经登记好，插件名称冲突检查能看到它们。

## 3. 第二步：检查命令绑定权

[CommandBindings.cpp](C:/Users/35536/Desktop/LearnFramework/LearnFramework/src/runtime/CommandBindings.cpp) 创建堆上的 PluginLoader，然后尝试注册命令 1000。

注册成功后，处理器的寿命交由总线管理。失败时，CommandBindings 自己删除尚未被总线接管的加载器，registered 保持 false。

ApplicationHost::show 首先检查这个标记。若另一个宿主已占用 1000，新宿主拒绝启动。它不能借用别人的加载器，也不能在析构时把别人的命令注销掉。

这体现了一个通用原则：只有取得资源管理权的对象，才负责归还该资源。

## 4. 第三步：定位并打开 UI 文件

UI 路径由 `QCoreApplication::applicationDirPath()` 加上 `ui/main.ui` 得到。

假设启动的程序是：

```text
项目/bin/LearnFramework.exe
```

那么读取的是：

```text
项目/bin/ui/main.ui
```

从哪个工作目录启动不影响这个路径。程序也不会直接去源码 ui 目录读取。这个区别是“修改 UI 却没有变化”的常见原因。

源码先尝试打开文件，打开失败立即结束 show；成功后才扫描插件。**打开 UI 文件**和**解析并创建 UI**是两个不同步骤。

## 5. 第四步：发出核心命令 1000

源码片段：

```cpp
ServiceRegistry::instance()->commandBus()->dispatchCommand(
    1000, path.data(), path.size());
```

path 是插件目录的 UTF-8 字节序列。调用链如下：

```text
ApplicationHost::show
  → CommandBus::dispatchCommand
    → 查找编号 1000 的处理器
      → PluginLoader::dispatchCommand
        → loadDirectory
          → 逐个 loadPlugin
```

CommandBus 不知道什么叫 DLL，它只根据编号找到 CommandHandler，然后调用虚函数。PluginLoader 才负责解释路径和加载库。

路径缓冲区在这次同步调用期间有效。加载器把字节转成 QString 后使用，不把原始 char 指针留到以后。

## 6. 第五步：解析 UI，再填充内容

所有候选插件处理完毕后，主程序建立 RuntimeUiLoader：

1. 设置相对资源路径的基准目录。
2. `load(&file)` 解析 XML，创建根 QWidget、布局、分组框及 WidgetContainer。
3. 将根窗口交给 unique_ptr。
4. `populateContainers` 读取各容器的 contentClass，调用工厂创建内容。
5. 全部成功后显示根窗口。

为什么必须先加载插件？每个容器会保存工厂副本；如果先创建容器，再给原工厂添加插件类型，旧副本不会自动获得这些新登记。

## 7. 第六步：进入交互阶段

show 成功返回后才执行 app.exec。之后鼠标点击触发 Qt 信号，信号调用业务函数，业务函数可能同步调用命令总线。

再次对同一个已建立窗口的宿主调用 show，只会显示已有窗口，不会重新读 UI 或扫描插件。修改磁盘文件要重启才能通过正常启动流程重新加载。

## 8. 启动失败如何传播

| 发生的情况 | 实际行为 |
| --- | --- |
| 命令 1000 已被占用 | show 返回 false |
| UI 无法打开 | show 返回 false，尚未扫描插件 |
| 某个候选插件失败 | 扫描继续，宿主记录警告，仍尝试加载 UI |
| UI XML 解析失败 | show 返回 false |
| 容器请求的名称不存在或创建返回空 | 释放根窗口，show 返回 false |
| 插件目录为空或不存在 | 扫描本身返回成功；UI 是否能填充仍需另判断 |

因此“单个插件失败”不一定等于“程序必须失败”。如果 UI 没有依赖它，程序仍可能成功显示。默认六面板 UI 依赖三个内容插件，缺少其中一个则对应区域无法创建。

## 9. 建议设置的断点

按顺序设置：

1. main 中构造 core 的那一行。
2. CommandBindings 的 registerHandler。
3. ApplicationHost::show 的 dispatchCommand。
4. PluginLoader::initializePlugin。
5. RuntimeUiLoader::createWidget。
6. WidgetContainer::setContentClass。

观察何时仅登记函数，何时才出现 QWidget 实例。调试器跨入 Qt 库细节时可先“单步跳过”，把注意力留在项目自己的调用关系。

本章自检：为什么 app.exec 前就能完成面板创建和初始设备状态查询？因为这些初始化和总线调用都是同步执行的；事件循环负责后续事件处理，不是所有函数运行的前提。

