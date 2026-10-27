# 09 构建、练习、测试与设计边界

[返回目录](README.md) · [上一章](08-lifetime.md)

## 1. 先选对工程

用 Qt Creator 打开 [Framework.pro](C:/Users/35536/Desktop/LearnFramework/LearnFramework/Framework.pro)，选择兼容的 Qt 5 Kit，构建后运行 LearnFramework 目标。

Framework.pro 是 subdirs 总工程，包含 application 和四个插件子工程。插件的 depends 指向 application，规定构建顺序，不表示这些 DLL 链接主程序中的业务实现。

[Learn.pro](C:/Users/35536/Desktop/LearnFramework/LearnFramework/Learn.pro) 只构建主程序，使用 core、widgets、uitools 模块；各插件 .pro 使用 TEMPLATE=lib 与 CONFIG+=plugin 生成动态库。插件也编译公共 Plugin.cpp，提供接口析构函数实现，不需要向主程序索取该符号。

Kit 可以理解为配套的 Qt 版本、编译器、调试器和目标架构。主程序和插件应使用匹配的 Qt、编译器、位数及 Debug/Release 配置。不要只看文件扩展名都是 DLL 就认为可以混用。

## 2. 输出与部署关系

默认目录关系：

```text
项目根目录/
  Framework.pro
  Learn.pro
  ui/main.ui                 ← 源码中的设计文件
  bin/
    LearnFramework.exe
    ui/main.ui               ← 实际启动时读取
    plugins/
      DemoPlugin.dll
      ContentPlugin.dll
      DevicePlugin.dll
      ReportPlugin.dll
```

Learn.pro 的部署目标将源码 UI 复制到输出目录。修改源码 UI 后需要让构建部署步骤执行；直接修改 bin/ui/main.ui 可以在重启时验证，但之后重新构建可能覆盖该文件。需要保留的设计应同步回源码版本。

这里列的是项目自己的输出；在没有对应 Qt 运行环境的机器上分发，还需配套的 Qt 运行库、平台插件和编译器运行依赖。项目的 UI 复制步骤并不等于完整安装包制作。

当前环境已有 bin/build 产物，但本次没有据此推断它们一定对应当前源码，也没有重新编译。当前终端没有找到 PATH 中的 qmake；这不代表机器没有 Qt，可在 Qt Creator 的 Kit 设置中查看实际工具路径。

## 3. 练习一：只改布局配置

目标：理解“不改 C++，选择已有内容”。

1. 关闭运行中的程序。
2. 备份运行目录 ui/main.ui。
3. 用 Designer 修改 contentCounterContainer 的动态字符串属性 contentClass，从 plugin.f 改为 c。
4. 保存，重新运行。
5. 预期左上显示内置计数器，原有分组标题仍可能是“内容 · 计数”。
6. 恢复 UI 备份。

这里改的是内容，分组标题由另一个 UI 属性控制。如果输入未登记名称，预期启动失败并输出 Cannot populate UI，而不是自动退回默认面板。

## 4. 练习二：用断点观察跨插件调用

目标：区分命令请求、状态修改和消息广播。

依次在以下函数设断点：

1. FWidget::requestDeviceConnection。
2. PluginServicesScope::sendCommand。
3. DevicePlugin::handleSetConnection。
4. DevicePlugin::publishState。
5. observeDevice 注册的回调内部。

点击连接，观察 data 的 connected 为 bool。沿调用栈返回，确认接收标签更新发生在发送函数返回前。不要在这个练习中加入 sleep 来“看清效果”，它会阻塞 GUI 线程，改变使用体验。

## 5. 练习三：增加一个内置面板

这是建议你动手做的练习，文档没有修改现有业务源码。

最小修改路径：

1. 在 DemoWidgets.h 声明一个继承 QWidget 的新类型。
2. 在 DemoWidgets.cpp 实现构造函数，设置布局并添加标签。
3. 在 registerDemoWidgets 注册一个新名称。
4. 重新编译主程序。
5. 在 UI 的容器中选择新名称，重启观察。

教学示意（需要放在正确文件并补齐相应声明和 include）：

```cpp
class HelloWidget : public QWidget
{
public:
    explicit HelloWidget(QWidget *parent = nullptr);
};

// DemoWidgets.cpp 中，已有 QWidget/QLabel/QVBoxLayout 的相关包含。
HelloWidget::HelloWidget(QWidget *parent) : QWidget(parent)
{
    auto *layout = new QVBoxLayout(this);
    layout->addWidget(new QLabel(QStringLiteral("我的第一个面板"), this));
}

// 加入 registerDemoWidgets 函数体：
factory.registerWidget("hello", [](QWidget *parent) {
    return new HelloWidget(parent);
});
```

检查结果：容器选择 hello 能显示；两个容器同时选择 hello 得到两个实例；改成未知名称会走失败路径。这比一开始写复杂设备通信更适合建立信心。

## 6. 练习四：在现有插件内增加面板

理解内置面板后，再尝试：

1. 在 ContentPlugin 的 widgets 目录增加新 QWidget 类。
2. 把新 .h/.cpp 加到 ContentPlugin.pro。
3. widgetClasses 增加唯一键，例如 plugin.hello。
4. createWidget 增加对应分支，返回一个带 parent 的新窗口。
5. 关闭主程序，用匹配 Kit 重新构建 ContentPlugin。
6. 更新运行目录 UI 的 contentClass，重启。

这个练习不需要改主程序的类型判断，因为 PluginLoader 会读取新的名称列表。修改 .pro 后在 Qt Creator 重新运行 qmake，确保新文件进入构建。

如果希望做一个全新 DLL，可以参考 ReportPlugin 的工程结构，修改目标名、入口类名和内容键；独立构建后放入 plugins，或把新子工程加入 Framework.pro。原有 Plugin 接口 IID 要与宿主期待的接口保持一致，不要仅为了换业务名称就任意修改接口 IID。

## 7. 现有测试怎么读

测试通常包含三段：准备环境 → 执行动作 → 检查结果。

例如 WidgetPluginSmokeTests 创建宿主，找到 pluginCounterButton，QTest::mouseClick 模拟点击，再 QCOMPARE 按钮文字。这样验证的是从真实 DLL 到真实控件的完整连接，而不是只检查一个孤立计数函数。

| 测试工程 | 重点 | 输出位置/准备 |
| --- | --- | --- |
| DemoPluginSmokeTests.pro | 命令 1000、工具窗口显示、两次点击、清理 | 默认项目 bin，提前构建 DemoPlugin |
| ApplicationHostUiTests.pro | UI 重载、路径独立、缺失/损坏文件、容器替换 | 构建目录 ui-test-bin |
| WidgetPluginSmokeTests.pro | 实际内容 DLL、六面板、共享状态、通信、生命周期与回滚 | 构建目录 plugin-test-bin，复制三个内容 DLL 到其 plugins |

Framework.pro 没有包含这些测试子工程，需要单独打开对应 .pro 构建。

WidgetPluginSmokeTests 的 initTestCase 按 .dll 文件名检查 ContentPlugin、DevicePlugin、ReportPlugin，测试准备带有 Windows 假设。其清理断言要求没有多余顶层窗口，因此测试插件目录建议只放它要求的三个内容插件，避免 DemoPlugin 创建额外工具窗口干扰结果。

测试会生成并删除输出目录里的临时 UI，不要把 UI 测试输出指向正式运行目录。UI 和插件测试默认已经使用独立目录。

如果可执行文件及 Qt 运行环境已准备好，可以在测试程序所在目录运行：

```powershell
.\ApplicationHostUiTests.exe -platform offscreen
.\WidgetPluginSmokeTests.exe -platform offscreen
```

两条命令分别在各自的输出目录执行。offscreen 适合不展示窗口的逻辑检查，并不能代替真实屏幕的外观检查；测试环境还需要相应 Qt 平台插件。

## 8. 几个值得阅读的测试用例

在 [WidgetPluginSmokeTests.cpp](C:/Users/35536/Desktop/LearnFramework/LearnFramework/tests/WidgetPluginSmokeTests.cpp) 中：

- factoryCopiesKeepPluginAliveAndDestroyContentBeforePlugin：证明工厂副本与插件寿命的关系。
- rejectsDuplicateNamesWithoutPartialRegistration：冲突时没有留下半套名称。
- secondHostDoesNotRemoveFirstHostsCommandBinding：失败宿主不能注销成功宿主的核心命令。
- communicatesBetweenContentAndDevicePlugins：两个订阅面板共同更新，动态移除订阅者后仍可通信。
- servicesCleanUpCommandsAndSubscriptions：owner 销毁、作用域清理、分发中删除订阅者与自注销命令。
- failedPluginInitializationRemovesOnlyItsOwnCommands：初始化失败清理自己的部分注册，保留既有处理器。

这些是测试源码表达的预期，不表示本次已经运行并通过。测试也不是“所有异常情况都已覆盖”的证明。

## 9. 常见问题沿哪条链排查

| 现象 | 优先检查 |
| --- | --- |
| 修改 UI 没变化 | 是否修改了运行程序旁的 UI；是否重启；是否被构建覆盖 |
| Cannot open UI | 日志中的实际路径是否存在且可读 |
| Cannot load UI | XML 是否损坏、控件描述是否有效 |
| Cannot populate UI | contentClass 是否与已登记名称完全一致 |
| Cannot load Plugin | DLL 是否为兼容 Qt 插件、依赖是否齐全，查看 errorString |
| Some plugins failed to load | 往前找具体 DLL 或名称冲突日志 |
| 设备服务不可用 | 2000/2001 是否注册；DevicePlugin 初始化是否失败 |
| 设置输入了但监控没更新 | 是否点击应用，名称是否为空 |
| 新面板没有消息 | 是否实现通信接口、使用有效 owner、先订阅再查询 |
| 第二个 ApplicationHost 启动失败 | 1000 是否被第一个宿主占用 |
| 窗口关闭时崩溃 | 借用指针、回调上下文和模块释放顺序 |

不要先盲目改总线。按日志和调用链缩小问题，通常能更快找到“文件路径错误”“名称错误”或“初始化失败”。

## 10. 当前设计有哪些优点

接口把宿主与具体业务分开；工厂统一内置和 DLL 内容；UI 让布局选择独立于代码；通信作用域让注册清理有明确归属；创建新内容成功后才删除旧内容；真实 DLL 测试覆盖了部分容易出错的生命周期情况。

这些优点都对应具体代码路径，不需要靠记住大量设计模式名称来理解。

## 11. 当前局限与改进方向

下表是源码分析结论和后续建议，并非本次已经修改的内容。

| 当前实现/边界 | 影响 | 可考虑的改进 |
| --- | --- | --- |
| 底层命令使用裸指针和长度 | 类型约定由调用双方保证 | 逐步采用类型明确的载荷接口 |
| 底层 registerHandler 未验证空指针 | 不符合调用约定时可能留下异常条目 | 入口检查并明确返回值 |
| 命令区间循环没有完整边界校验 | unsigned 最大值可能回绕；从 0 首项冲突时回滚会下溢 | 校验区间，按已成功 ID 列表回滚 |
| 底层消息订阅不拥有对象 | 原始订阅快照中对象可失效 | 业务继续走带 owner 的服务层，或改造底层寿命约束 |
| ServiceRegistry 记录借用裸指针 | 对象销毁后条目可能悬空 | 使用 QPointer 并补充注销接口 |
| 通信返回 bool | 缺失处理器与载荷错误难区分 | 定义结构化错误结果 |
| 初始化及回调没有统一异常捕获 | 抛异常可能跳过预期错误报告 | 规定接口不抛异常，或设计明确捕获边界 |
| 单 GUI 线程同步调用 | 慢任务会卡界面 | 明确工作线程与 GUI 线程之间的数据交接 |
| 状态字段公开，通知手工发出 | 新增代码可能忘记通知 | 集中状态更新入口 |
| 无持久化、真实采集、依赖版本协商 | 无法直接视为完整业务产品 | 按实际需求分别设计，而非默认已有 |

其中无符号边界问题可以直接从 CommandBus 的闭区间 for 循环看出：当末项等于 unsigned int 最大值，递增会回到 0，条件无法正常结束。学习时应理解这个缺口，不要通过运行极大范围来尝试验证。

插件加载并不是安全隔离：代码在同一进程执行。这里讨论的是可维护性和生命周期管理，没有实现插件崩溃隔离或权限沙箱。

## 12. 建议的学习节奏

第一轮只掌握 main、ApplicationHost、内置计数面板，能跟踪一次点击。

第二轮修改 contentClass，追踪工厂回调，再读 DemoPlugin 与 ContentPlugin。

第三轮画出 2000/3000 调用链，观察共享状态与独立窗口。

第四轮研究所有权，阅读现有测试，再考虑增加一个新面板或新协议。

每一轮结束时，用自己的话说明“输入是什么、哪个对象处理、数据存在哪里、结果在哪里显示、最后谁清理”。这比逐行背代码更能检验理解。

## 13. 本次文档工作的验证范围

本次阅读并交叉核对了运行入口、公共接口、总线、插件加载与通信实现、窗口模块、四个示例插件、UI、qmake 配置和相关测试源码；新增学习文档，没有修改业务实现。

检查文档内部链接、源码链接、代码块配对及 Git 变更范围。没有重新构建 C++、执行 Qt 测试或验证新增练习代码的编译结果，因此文中的运行结果属于基于源码的预期，供你实际练习时核对。

源码入口：[UI 测试](C:/Users/35536/Desktop/LearnFramework/LearnFramework/tests/ApplicationHostUiTests.cpp)、[最小插件测试](C:/Users/35536/Desktop/LearnFramework/LearnFramework/tests/DemoPluginSmokeTests.cpp)、[底层总线](C:/Users/35536/Desktop/LearnFramework/LearnFramework/src/foundation/CommandBus.cpp)。

