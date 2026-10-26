# 纯框架学习版

保留 Qt 启动、外部 UI 加载、可配置窗口容器、命令/消息分发、插件接口与加载、通用窗口和共享对象注册。

已移除登录、通信服务、共享业务数据、设备协议、业务 ID、业务配置，以及站位、菜单和登录布局。原项目没有修改。

框架统一采用独立命名：类名使用 PascalCase，方法使用 lowerCamelCase。源码按职责分为 runtime（应用启动和插件加载）与 foundation（基础接口与通用服务）。不提供旧接口的兼容别名。

## 学习顺序

main.cpp（QApplication）→ ApplicationHost::show → CommandBus::dispatchCommand → PluginLoader::dispatchCommand → Plugin::initialize。ApplicationHost 构造时，CommandBindings 先完成处理器注册。

CommandBindings 注册命令 1000，ApplicationHost 发出命令，分发器找到加载器，再由加载器调用插件初始化。插件若实现 WidgetProvider，则在 UI 创建之前将内容名称注册到窗口工厂。内容窗口先销毁，再释放工厂回调并关闭插件。

附带 DemoPlugin 示例：加载后显示一个带按钮的小窗口，点击按钮更新计数。插件接口保留四阶段，加载器目前仅调用初始化与关闭；消息分发没有业务订阅者。

## 工程

使用 Qt 5 的 Qt Creator 打开 Framework.pro，同时构建主程序、DemoPlugin、ContentPlugin、DevicePlugin 和 ReportPlugin，并选择 LearnFramework 作为运行目标。主程序输出到 bin，插件自动输出到 bin/plugins，无需手动复制 DLL。主程序和插件须使用同一个 Kit、相同 Debug/Release 配置。

Learn.pro 仍可单独构建主程序，各插件的 .pro 也可独立构建。默认六分区 UI 需要 ContentPlugin、DevicePlugin、ReportPlugin 三个 DLL；单独构建主程序时请另外构建或部署它们。插件源码放在 examples 下，生成的动态库放在可执行文件旁的 plugins 目录。插件不是独立可执行程序，构建后应运行 LearnFramework。

命令 1000 表示“扫描目录并加载插件”，不是 DemoPlugin 的专属编号。调用链为 ApplicationHost::show → CommandBus::dispatchCommand(1000) → PluginLoader → DemoPlugin::initialize。初始化创建窗口和按钮；退出主程序时 shutdown 删除窗口。插件不访问 ServiceRegistry，也不链接主程序，适合作为第一个动态插件示例。

可选验证：先构建示例插件，再用相同 Kit 和构建配置打开 tests/DemoPluginSmokeTests.pro，运行测试。它通过真实命令 1000 加载 DLL，检查窗口显示、两次点击后的计数，以及退出时的窗口释放。无桌面环境可传入参数 `-platform offscreen`。

注册表不销毁窗口和共享服务。命令处理器通过分发器的注册引用计数管理，消息订阅不持有对象。原分发器的局限留待后续分析。

## 外部 UI（修改后重启生效）

ApplicationHost 使用 RuntimeUiLoader（继承 QUiLoader）在启动时读取可执行文件旁的 `ui/main.ui`，通过 `std::unique_ptr<QWidget>` 管理窗口，不再声明 `QMainWindow window`。窗口类型、标题、尺寸及布局由 UI 文件决定；默认 UI 使用 2 行 × 3 列六个内容容器，每列由一个独立 DLL 提供上下两个界面。

首次修改 C++ 后需要重新构建一次（Learn.pro 已增加 `QT += uitools`，请重新运行 qmake）。构建会把源码 `ui/main.ui` 复制到 `bin/ui/main.ui`。之后用 Qt Designer 打开并保存 **`bin/ui/main.ui`**，关闭并重新启动 `bin/LearnFramework.exe` 即可看到变化，不需要编译，也不需要从 Qt Creator 启动。

源码 `ui/main.ui` 是构建时部署的版本；重新构建会覆盖运行目录的 UI，如需保留设计，请同步保存到源码版本。分发程序时需要携带 `ui` 目录。加载路径不受启动工作目录影响；相对图片路径以 UI 文件所在目录为基准。UI 缺失或解析失败时记录错误并以状态码 1 退出。

控件事件仍由 C++ 连接，可在加载后使用 `window->findChild<QPushButton *>("按钮的objectName")` 获取控件。修改界面不需要编译；新增业务逻辑或自定义控件仍需代码支持。此方案在重新启动时读取文件，不会自动刷新已经打开的窗口。

验证工程 `tests/ApplicationHostUiTests.pro` 使用独立测试目录，检查 UI 修改后重新加载、控件文字、不同工作目录、窗口释放及缺失/损坏文件；可使用 `-platform offscreen` 运行。实现使用 Qt 官方的 [QUiLoader](https://doc.qt.io/archives/qt-5.15/quiloader.html)。

## 可配置 QWidget 容器

`WidgetContainer` 对应通用外壳 b；`CWidget`、`DWidget`、`EWidget` 是三个独立的 QWidget 子类，分别演示计数、文本输入和列表。WidgetFactory 注册配置名 `c`、`d`、`e`，每次加载都创建独立实例，因此两个容器都选 `c` 时计数也互不影响。

默认 `main.ui` 的六个区域如下：

| 位置 | 容器 objectName | contentClass | 提供者 |
| --- | --- | --- | --- |
| 左上：内容计数 | contentCounterContainer | plugin.f | ContentPlugin.dll |
| 左下：内容文本 | contentTextContainer | plugin.g | ContentPlugin.dll |
| 中上：设备监控 | deviceMonitorContainer | device.monitor | DevicePlugin.dll |
| 中下：设备设置 | deviceSettingsContainer | device.settings | DevicePlugin.dll |
| 右上：报表预览 | reportViewerContainer | report.viewer | ReportPlugin.dll |
| 右下：报表编辑 | reportEditorContainer | report.editor | ReportPlugin.dll |

用 Designer 打开 **运行目录的 `ui/main.ui`**，选择某个容器，在属性编辑器中修改动态字符串属性 `contentClass`，保存并重启程序即可切换，不需要编译。六个位置都可以选择任意已注册名称，包括内置的 `c`、`d`、`e`。

DevicePlugin 内部的监控与设置共享设备名称、刷新间隔和模拟连接状态。点击设置面板的“应用配置”，监控面板立即同步；设备连接只做模拟，刷新间隔是配置演示，不启动硬件通信或定时采集。ReportPlugin 内部的编辑器与预览共享报表文本，点击“更新预览”生效。设备配置和报表内容仅在本次运行内保存；两个业务插件不依赖彼此，也不链接主程序。

在 Designer 中新增容器：拖入 QWidget，右键“提升为”，提升类名填 `WidgetContainer`，头文件填 `WidgetContainer.h`，添加并提升。然后在属性编辑器中点击“+”，添加 QString 类型的动态属性 `contentClass`。Designer 中容器显示为占位区域，实际内容在程序运行时创建；容器内部布局由代码管理，不要在其中手动添加子控件或布局。

UI 中的配置形式如下（`stdset="0"` 表示动态属性）：

```xml
<widget class="WidgetContainer" name="contentCounterContainer">
 <property name="contentClass" stdset="0">
  <string>d</string>
 </property>
</widget>
```

`RuntimeUiLoader::createWidget` 识别容器类，完整加载 UI 后再根据 `contentClass` 填充内容。未设置或为空时保留空容器；未知类名会记录具体容器和配置名并终止启动，已创建的窗口会释放。容器使用零边距布局让内容填满，父子关系保证退出时释放内容。

运行时也可以在持有主窗口的代码中切换：

```cpp
auto *container = window->findChild<WidgetContainer *>("contentCounterContainer");
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

内置 C++ 类需要编译进主程序；之后在 UI 中选择已注册类不需要编译。若希望新增类也不用重编译主程序，可使用下面的 WidgetProvider 插件接口。DemoPlugin 继续保留独立窗口示例。

相关实现位于 `src/widgets`；`ApplicationHostUiTests` 另外验证左右容器配置、同类实例独立、重启切换、替换/清空及失败时保留原窗口。

## 用插件新增界面，无需重编译主程序

新增公共接口 `src/foundation/WidgetProvider.h`，插件同时实现原有 `Plugin` 生命周期接口和可选的 `WidgetProvider` 能力：

```cpp
class MyPlugin : public QObject, public Plugin, public WidgetProvider
{
    Q_OBJECT
    Q_PLUGIN_METADATA(IID "org.learnframework.Plugin/1.0")
    Q_INTERFACES(Plugin WidgetProvider)
    // 实现 initialize/start/stop/shutdown。
    // 实现 widgetClasses() 和 createWidget(name, parent)。
};
```

`widgetClasses()` 返回该插件提供的配置名称；`createWidget` 按名称返回一个新的 QWidget 子类实例，传入 parent，失败返回 nullptr。插件依赖公共接口与 Qt，不链接主程序，也不调用 ServiceRegistry 或 WidgetFactory。插件自身的资源在 initialize 中准备，在 shutdown 中释放；需要跨插件通信时额外实现下面的可选 PluginServicesAware 接口。

完整示例分别在 `examples/ContentPlugin`、`examples/DevicePlugin`、`examples/ReportPlugin`。它们独立编译为三个 DLL，各提供两个 QWidget 子类。主程序没有包含这些业务类的实现，也没有硬编码注册它们的名称。

ContentPlugin 内部按职责拆分，阅读时可以直接定位：

- `ContentPlugin.h/.cpp`：插件生命周期、提供的名称及按名称创建窗口。
- `widgets/FWidget.h/.cpp`：计数面板与设备连接命令按钮。
- `widgets/GWidget.h/.cpp`：文本输入面板与独立的状态显示。
- `widgets/DeviceStatusBinding.h/.cpp`：两个面板共用的状态订阅、标签更新和初始状态查询。

DevicePlugin 同样按职责拆分：

- `DevicePlugin.h/.cpp`：插件生命周期、命令注册、状态消息发布及按名称创建窗口。
- `DeviceState.h`：监控和设置面板共享的设备状态，以及 `changed` 通知信号；由插件拥有。
- `widgets/DeviceMonitorWidget.h/.cpp`：设备状态显示和模拟连接切换。
- `widgets/DeviceSettingsWidget.h/.cpp`：设备名称、刷新间隔的输入校验和应用配置。

两个面板只依赖 DeviceState，不直接访问主程序通信服务；状态改变后，由插件入口统一发布跨插件消息。

1. 首次使用此机制时构建一次更新后的主程序。使用 Framework.pro 可同时构建默认六分区 UI 所需的全部插件。
2. 用相同 Qt、编译器、位数和 Debug/Release 配置，单独打开各插件的 .pro 也能独立构建。默认输出到 `bin/plugins`。
3. 在程序旁 `ui/main.ui` 中选择已注册的 contentClass，保存并重新启动程序。
4. 新增业务类时只修改并编译插件 DLL，再更新 UI 配置。无需重新编译或重新链接主程序。

一个 DLL 可以提供任意多个内容类；同一名称可以装入多个容器，各自创建独立实例。推荐使用 `厂商或插件名.界面名` 形式，避免与其他插件或内置类型重名。所有名称先验证再注册，空名称、插件内部重名或与已有类型重名都会拒绝整个插件，保留已有注册。重复扫描同一个成功加载的 DLL 不会重复初始化。

启动顺序为：注册内置类 → 扫描插件并初始化、注册插件类 → 加载 UI 并创建容器 → 按 contentClass 创建内容。单个插件失败会记录日志，其他有效插件仍可使用；如果 UI 选择的类型因此不存在，启动失败并退出。

容器复制工厂时会保留插件模块的共享引用，确保构造函数不会引用已关闭的插件。退出时先释放 UI/内容，再清除工厂回调，最后调用插件 shutdown 和 QPluginLoader::unload。加载器也记录插件创建的残留窗口，在 shutdown 前释放。Qt 的默认 PreventUnloadHint 保留，因此不保证进程运行中物理卸载 DLL。

本机制在启动时发现 DLL；不支持程序运行中热替换 DLL。部署新插件时应先关闭程序，再替换 DLL 并重启。首次增加插件机制需要更新主程序；后续插件遵守 `Plugin/1.0`、`WidgetProvider/1.0` 接口即可独立扩展，接口或工具链变更时需重建兼容插件。

验证：构建三个内容插件后，构建 `tests/WidgetPluginSmokeTests.pro`，将同配置的 ContentPlugin.dll、DevicePlugin.dll、ReportPlugin.dll 复制到测试输出目录 `plugin-test-bin/plugins`，运行测试。测试覆盖实际 ApplicationHost 加载 DLL 内容、业务交互、插件/内置内容切换、工厂副本生命周期、销毁顺序、重复扫描及名称冲突不产生部分注册；还会读取真实的源码 main.ui，验证六面板同时加载、设备配置同步、报表编辑预览同步、窗口缩放和退出清理，生成 six-panels-preview.png 供检查。

## 跨插件命令与消息演示

打开程序后，左上“内容 · 计数”面板提供“发送连接命令”和“发送断开命令”。点击后，ContentPlugin 通过主程序总线发出请求，DevicePlugin 修改模拟设备状态，再发布状态消息；中上监控面板和左侧两个内容面板都会更新。

直接点击设备监控面板的模拟连接按钮、或在设备设置面板应用名称和刷新间隔，也会发布状态消息。左下文本面板是第二个独立订阅者，用来演示一条消息通知多个界面。内容面板显示收到的消息序号，窗口创建时会主动请求当前状态，避免等到下一次操作才显示设备状态。

两个 DLL 不包含彼此的业务类、不互相链接，也不把 ServiceRegistry 编译到插件中。它们只共享：

- `PluginServices.h`：主程序提供的通信接口及可选的 PluginServicesAware 接口。
- `DeviceProtocol.h`：设备命令、消息编号和载荷约定。

| 编号 | 类型 | 载荷 |
| --- | --- | --- |
| 2000 | SetConnection 命令 | connected：bool |
| 2001 | QueryStatus 命令 | 无；设备通过状态消息返回当前状态 |
| 3000 | StatusChanged 消息 | connected：bool、name：QString、refreshInterval：int |

主程序在 initialize 前调用 `setPluginServices` 注入每个插件独立的服务作用域。插件通过服务注册命令、发送命令、订阅消息和发布消息：

```cpp
// 插件声明可选能力：public PluginServicesAware
// Q_INTERFACES(Plugin WidgetProvider PluginServicesAware)
void setPluginServices(PluginServices *value) override { services = value; }

// ContentPlugin：发送命令，bool 表示是否被成功处理。
QVariantMap request;
request.insert("connected", true);
bool handled = services->sendCommand(DeviceProtocol::SetConnection, request);

// 窗口订阅：this 是接收窗口，销毁时自动断开。
services->subscribe(DeviceProtocol::StatusChanged, this,
    [this](const QVariantMap &data) { /* 更新界面 */ });
```

`PluginMessaging` 适配现有 CommandBus：命令交给单个处理器，消息经主程序共享的中转对象交给 Qt 连接分发。每个命令或订阅必须指定一个 QObject 所有者。所有者销毁时命令自动注销、订阅自动断开；插件 shutdown 后会撤回接口并清除该插件的全部回调，再卸载插件。初始化失败、命令编号冲突时也会清理已注册的部分，已有服务不受影响。

插件编号使用 2000 及以上，避免访问核心的命令 1000。空回调、同名命令、错误载荷和不存在的处理器会返回失败。消息载荷使用 QVariantMap，不向插件暴露原有 char*/void* 缓冲区。接口是同步 GUI 线程调用，不提供后台线程或跨进程通信；回调中若需要长期保存数据，请复制载荷，不要保留参数引用。

原有 Plugin/1.0、WidgetProvider/1.0 接口未改变，DemoPlugin 和 ReportPlugin 不必实现通信能力。初次启用需要重新构建主程序及 ContentPlugin、DevicePlugin；后续其他插件可通过可选接口接入。若缺少设备命令处理器，ContentPlugin 会明确显示“设备服务不可用／命令未处理”；默认六分区 UI 本身仍需要三个内容插件齐全。

WidgetPluginSmokeTests 额外验证双向通知、两个订阅者同时更新、动态移除订阅面板、命令/订阅自动清理、分发中销毁订阅者、自注销命令、缺失设备服务及初始化失败后的回滚，并生成 plugin-communication-preview.png。

## 接口约定

- ApplicationHost：组织应用窗口与启动流程。
- CommandBindings：注册并注销核心命令处理器。
- CommandHandler：定义 dispatchCommand、dispatchMessage 和 dispose。
- CommandBus：提供 registerHandler、unregisterHandler、dispatchCommand、subscribe、unsubscribe、dispatchMessage。
- PluginLoader：扫描动态库、初始化插件，并注册可选的 WidgetProvider 内容。
- Plugin：定义 initialize、start、stop、shutdown 四个生命周期方法；接口标识为 org.learnframework.Plugin/1.0。
- WidgetProvider：声明 widgetClasses 和 createWidget；接口标识为 org.learnframework.WidgetProvider/1.0。
- PluginServicesAware：可选通信能力，接口标识 org.learnframework.PluginServicesAware/1.0；PluginServices 由主程序注入。
- ServiceRegistry：通过 instance 获取实例，提供 commandBus、registerWindow、findWindow、registerService、findService。

此次增加 WidgetProvider 是可选能力，原有 Plugin/1.0 接口未改变；使用同一 Kit 构建的旧版 DemoPlugin 可继续加载。
