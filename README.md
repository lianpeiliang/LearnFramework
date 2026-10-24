# 纯框架学习版

仅保留 Qt 启动、空主窗口、命令/消息分发、插件接口与加载、通用窗口和共享对象注册。

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
