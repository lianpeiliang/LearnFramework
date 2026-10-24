# 纯框架学习版

仅保留 Qt 启动、空主窗口、命令/消息分发、插件接口与加载、通用窗口和共享对象注册。

已移除登录、通信服务、共享业务数据、设备协议、业务 ID、业务配置，以及站位、菜单和登录布局。原项目没有修改。

框架统一采用独立命名：类名使用 PascalCase，方法使用 lowerCamelCase。源码按职责分为 runtime（应用启动和插件加载）与 foundation（基础接口与通用服务）。不提供旧接口的兼容别名。

## 学习顺序

main.cpp（QApplication）→ ApplicationHost::show → CommandBus::dispatchCommand → PluginLoader::dispatchCommand → Plugin::initialize。ApplicationHost 构造时，CommandBindings 先完成处理器注册。

CommandBindings 注册命令 1000，ApplicationHost 发出命令，分发器找到加载器，再由加载器调用插件初始化。加载器保存动态库对象，销毁时先关闭插件。

默认无插件，运行显示空主窗口。插件接口保留四阶段，目前仅调用初始化与关闭；消息分发没有业务订阅者。

## 工程

使用 Qt 5 的 Qt Creator 打开 Learn.pro。精简版合并为一个独立可执行工程，无需业务库。教学插件以后放在可执行文件旁的 plugins 目录，需匹配 Qt 与编译器。

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

当前源码及插件接口已改名，旧插件需要按新接口重新构建。此前的本地 Git 历史仍保存改名前的源码；分享历史仓库会同时包含旧名称。
