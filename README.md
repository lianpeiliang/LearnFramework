# 纯框架学习版

保留 Qt 启动、外部 UI 加载、可配置窗口容器、命令/消息分发、插件接口与加载、通用窗口和共享对象注册。

已移除登录、通信服务、共享业务数据、设备协议、业务 ID、业务配置，以及站位、菜单和登录布局。原项目没有修改。

框架统一采用独立命名：类名使用 PascalCase，方法使用 lowerCamelCase。源码按职责分为 runtime（应用启动和插件加载）与 foundation（基础接口与通用服务）。不提供旧接口的兼容别名。

## 学习顺序

main.cpp（QApplication）→ ApplicationHost::show → CommandBus::dispatchCommand → PluginLoader::dispatchCommand → Plugin::initialize。ApplicationHost 构造时，CommandBindings 先完成处理器注册。

CommandBindings 注册命令 1000，ApplicationHost 发出命令，分发器找到加载器，再由加载器调用插件初始化。加载器保存动态库对象，销毁时先关闭插件。

附带 DemoPlugin 示例：加载后显示一个带按钮的小窗口，点击按钮更新计数。插件接口保留四阶段，加载器目前仅调用初始化与关闭；消息分发没有业务订阅者。

## 工程

使用 Qt 5 的 Qt Creator 打开 Framework.pro，同时构建主程序与示例插件，并选择 LearnFramework 作为运行目标。主程序输出到 bin，插件自动输出到 bin/plugins，无需手动复制 DLL。主程序和插件须使用同一个 Kit、相同 Debug/Release 配置。

Learn.pro 仍可单独构建主程序；examples/DemoPlugin/DemoPlugin.pro 可单独构建插件。插件源码放在 examples 下，生成的动态库放在可执行文件旁的 plugins 目录。插件不是独立可执行程序，构建后应运行 LearnFramework。

命令 1000 表示“扫描目录并加载插件”，不是 DemoPlugin 的专属编号。调用链为 ApplicationHost::show → CommandBus::dispatchCommand(1000) → PluginLoader → DemoPlugin::initialize。初始化创建窗口和按钮；退出主程序时 shutdown 删除窗口。插件不访问 ServiceRegistry，也不链接主程序，适合作为第一个动态插件示例。

可选验证：先构建示例插件，再用相同 Kit 和构建配置打开 tests/DemoPluginSmokeTests.pro，运行测试。它通过真实命令 1000 加载 DLL，检查窗口显示、两次点击后的计数，以及退出时的窗口释放。无桌面环境可传入参数 `-platform offscreen`。

注册表不销毁窗口和共享服务。命令处理器通过分发器的注册引用计数管理，消息订阅不持有对象。原分发器的局限留待后续分析。

## 外部 UI（修改后重启生效）

ApplicationHost 使用 RuntimeUiLoader（继承 QUiLoader）在启动时读取可执行文件旁的 `ui/main.ui`，通过 `std::unique_ptr<QWidget>` 管理窗口，不再声明 `QMainWindow window`。窗口类型、标题、尺寸及布局由 UI 文件决定；默认 UI 包含左右两个内容容器。

首次修改 C++ 后需要重新构建一次（Learn.pro 已增加 `QT += uitools`，请重新运行 qmake）。构建会把源码 `ui/main.ui` 复制到 `bin/ui/main.ui`。之后用 Qt Designer 打开并保存 **`bin/ui/main.ui`**，关闭并重新启动 `bin/LearnFramework.exe` 即可看到变化，不需要编译，也不需要从 Qt Creator 启动。

源码 `ui/main.ui` 是构建时部署的版本；重新构建会覆盖运行目录的 UI，如需保留设计，请同步保存到源码版本。分发程序时需要携带 `ui` 目录。加载路径不受启动工作目录影响；相对图片路径以 UI 文件所在目录为基准。UI 缺失或解析失败时记录错误并以状态码 1 退出。

控件事件仍由 C++ 连接，可在加载后使用 `window->findChild<QPushButton *>("按钮的objectName")` 获取控件。修改界面不需要编译；新增业务逻辑或自定义控件仍需代码支持。此方案在重新启动时读取文件，不会自动刷新已经打开的窗口。

验证工程 `tests/ApplicationHostUiTests.pro` 使用独立测试目录，检查 UI 修改后重新加载、控件文字、不同工作目录、窗口释放及缺失/损坏文件；可使用 `-platform offscreen` 运行。实现使用 Qt 官方的 [QUiLoader](https://doc.qt.io/archives/qt-5.15/quiloader.html)。

## 可配置 QWidget 容器

`WidgetContainer` 对应通用外壳 b；`CWidget`、`DWidget`、`EWidget` 是三个独立的 QWidget 子类，分别演示计数、文本输入和列表。WidgetFactory 注册配置名 `c`、`d`、`e`，每次加载都创建独立实例，因此两个容器都选 `c` 时计数也互不影响。

默认 `main.ui` 的 `leftContainer` 配置 `contentClass=c`，`rightContainer` 配置 `contentClass=e`。用 Designer 打开 **运行目录的 `ui/main.ui`**，选择某个容器，在属性编辑器中把动态字符串属性 `contentClass` 改成 `c`、`d` 或 `e`，保存并重启程序即可切换，不需要编译。

在 Designer 中新增容器：拖入 QWidget，右键“提升为”，提升类名填 `WidgetContainer`，头文件填 `WidgetContainer.h`，添加并提升。然后在属性编辑器中点击“+”，添加 QString 类型的动态属性 `contentClass`。Designer 中容器显示为占位区域，实际内容在程序运行时创建；容器内部布局由代码管理，不要在其中手动添加子控件或布局。

UI 中的配置形式如下（`stdset="0"` 表示动态属性）：

```xml
<widget class="WidgetContainer" name="leftContainer">
 <property name="contentClass" stdset="0">
  <string>d</string>
 </property>
</widget>
```

`RuntimeUiLoader::createWidget` 识别容器类，完整加载 UI 后再根据 `contentClass` 填充内容。未设置或为空时保留空容器；未知类名会记录具体容器和配置名并终止启动，已创建的窗口会释放。容器使用零边距布局让内容填满，父子关系保证退出时释放内容。

运行时也可以在持有主窗口的代码中切换：

```cpp
auto *container = window->findChild<WidgetContainer *>("leftContainer");
QString error;
if (container && !container->setContentClass("d", &error))
    qWarning().noquote() << error;
```

替换成功会释放旧内容；创建失败时保留旧内容，重复设置相同类型不会重建。调用 `setContentClass("")` 可清空容器。运行时调用不会自动保存回 UI 文件；仅调用 QObject::setProperty 修改属性也不会自动切换，应使用 setContentClass。

添加新的内容类时，先实现 QWidget 子类，再在 `registerDemoWidgets`（或自己的注册函数）中注册构造函数：

```cpp
factory.registerWidget("myPanel", [](QWidget *parent) {
    return new MyPanel(parent);
});
```

新增 C++ 类需要编译一次；之后在 UI 中选择已注册类不需要编译。这些内容类目前编译在主程序中；现有 DemoPlugin 仍单独演示插件加载，不参与此工厂注册。

相关实现位于 `src/widgets`；`ApplicationHostUiTests` 另外验证左右容器配置、同类实例独立、重启切换、替换/清空及失败时保留原窗口。

## 接口约定

- ApplicationHost：组织应用窗口与启动流程。
- CommandBindings：注册并注销核心命令处理器。
- CommandHandler：定义 dispatchCommand、dispatchMessage 和 dispose。
- CommandBus：提供 registerHandler、unregisterHandler、dispatchCommand、subscribe、unsubscribe、dispatchMessage。
- PluginLoader：扫描动态库并初始化插件。
- Plugin：定义 initialize、start、stop、shutdown 四个生命周期方法；接口标识为 org.learnframework.Plugin/1.0。
- ServiceRegistry：通过 instance 获取实例，提供 commandBus、registerWindow、findWindow、registerService、findService。
- DesktopApplication：备用的 QApplication 包装，当前入口直接使用 QApplication。

当前源码及插件接口已改名，旧插件需要按新接口重新构建。
