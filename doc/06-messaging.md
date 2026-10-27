# 06 命令、消息与跨插件通信

[返回目录](README.md) · [上一章](05-plugins.md) · [下一章](07-business-state.md)

## 1. 先区分两种表达

**命令**表达“请执行某个动作”。例如设置连接状态，一个编号只有一个处理器，调用者得到成功或失败的返回值。

**消息**表达“这里有一份状态信息”。例如设备状态广播，一个编号可有多个订阅者，发布者不要求知道具体有哪些窗口在接收。

在本项目中，二者都是 GUI 线程上的同步调用。命令不是任务队列，消息也不是网络报文。

## 2. 两层接口不要混淆

| 层次 | 数据形式 | 生命周期处理 |
| --- | --- | --- |
| 底层 CommandBus | char* / void* 加 length | 命令引用计数；消息保存借用指针 |
| 插件 PluginServices | QVariantMap 和函数回调 | owner 销毁自动清理，加插件作用域集中清理 |

插件不直接使用底层的裸缓冲区接口。PluginMessaging 负责适配旧总线，让业务代码使用比较清楚的 map 和回调。

ServiceRegistry 在主程序提供全局总线入口；插件通过注入得到服务，而不是各自编译一个 ServiceRegistry。否则不同模块可能接入不同的注册表实例，不能自然保证通信都经过同一条总线。

## 3. CommandBus 内部的三张表

[CommandBus.cpp](C:/Users/35536/Desktop/LearnFramework/LearnFramework/src/foundation/CommandBus.cpp) 的逻辑可以写成：

```text
commandHandlers：命令编号 → 一个处理器
handlerReferences：处理器 → 成功命令绑定数量
messageSubscribers：消息编号 → 多个处理器指针
```

注册一条命令时增加处理器的引用计数；注销最后一条绑定时调用 dispose，默认执行 delete this。因此交给总线拥有的处理器应在堆上创建，不能传入一个等会还会自动析构的栈对象。

消息订阅不增加这个计数。原始订阅者必须在销毁前主动 unsubscribe。底层消息分发虽然复制订阅指针列表，复制的也只是地址，不是对象寿命。

## 4. 协议是双方共同知道的“词典”

[DeviceProtocol.h](C:/Users/35536/Desktop/LearnFramework/LearnFramework/src/foundation/DeviceProtocol.h) 定义：

| 编号 | 类型 | 字段/含义 |
| --- | --- | --- |
| 1000 | 框架核心命令 | UTF-8 插件目录路径，不属于 DeviceProtocol |
| 2000 | SetConnection | connected 必须为 bool |
| 2001 | QueryStatus | 查询当前状态，通过消息回复 |
| 3000 | StatusChanged | connected、name、refreshInterval |

ContentPlugin 不包含 DevicePlugin 的类定义。两者都知道 DeviceProtocol 和 PluginServices，于是一个发送“2000 + connected”，另一个理解这份约定。

这种解耦仍然需要共享协议：修改字段含义时必须考虑双方兼容，不能认为“没有 include 业务类”就等于没有依赖。

## 5. 一次连接按钮点击的完整链条

入口在 [FWidget.cpp](C:/Users/35536/Desktop/LearnFramework/LearnFramework/examples/ContentPlugin/widgets/FWidget.cpp)：

```text
用户点击“发送连接命令”
  → Qt clicked 信号
  → FWidget::requestDeviceConnection(true)
  → 构造 QVariantMap，放入 connected=true
  → PluginServicesScope::sendCommand(2000, data)
  → CommandBus::dispatchCommand(2000, ...)
  → PluginCommand::dispatchCommand
  → DevicePlugin 注册的 Lambda
  → DevicePlugin::handleSetConnection
  → state->connected = true
  → emit state->changed()
```

DevicePlugin 会检查 connected 的 QVariant 类型确实为 Bool。字符串 "true" 不等于这里要求的布尔载荷，缺失字段也会失败。

接下来，changed 触发两类接收逻辑：

```text
DeviceState::changed
  ├─ DeviceMonitorWidget::refreshState：刷新本地监控
  └─ DevicePlugin::publishState：广播完整状态
       → PluginServicesScope::publish(3000, data)
       → CommandBus::dispatchMessage
       → PluginMessageHub::dispatchMessage
       → messagePublished 信号
       → FWidget / GWidget 的状态订阅回调
       → 修改各自的状态标签
```

本地监控读共享 DeviceState；Content 面板读广播载荷。它们不是同一种数据访问路径。不要依赖图中两条分支的先后作为新的业务契约。

同步链条返回以后，FWidget 根据 handled 设置命令结果标签。也就是说，设备状态标签可能已经在 sendCommand 返回前完成更新。

## 6. 为什么需要 PluginCommand 适配器

CommandBus 要求 CommandHandler，插件提供的却是 std::function。PluginCommand 把插件函数包装成一个处理器，并用 QPointer 跟踪 owner。

sendCommand 复制一份 QVariantMap，然后把**对象地址**转成 char* 传给总线；适配器检查长度后转回 QVariantMap 指针。

这里的 sizeof(QVariantMap) 是 C++ 对象的大小，绝不是“所有键值序列化后的字节数”。没有编码、传输或反序列化过程。这套桥接只适合当前同进程同步约定，不能把这些字节直接发给另一个进程。

此外，适配器在调用前复制 callback。回调可能删除 owner，使当前处理器自动注销；持有函数副本避免正在执行的函数对象也立即随处理器销毁。

## 7. 为什么消息要经过 PluginMessageHub

底层总线不理解 QObject 的销毁状态。如果直接为每个面板注册裸 CommandHandler 指针，某个回调销毁另一个订阅窗口时，剩余快照可能含有悬空地址。

当前做法是：

1. 对每个消息编号，hub 只在总线订阅一次。
2. hub 收到消息后发出 Qt 信号。
3. 各窗口使用 owner 作为连接上下文。
4. owner 销毁后，Qt 自动断开相应连接。

hub 析构时主动从原始总线退订。这样，原始总线面对的是一个可控的中转对象，具体窗口的通知寿命由 Qt 连接管理。

所有消息共用一个 messagePublished 信号，各连接通过编号筛选。这种做法简单，订阅规模较大时也意味着更多筛选调用。

## 8. 每个插件的作用域如何清理

PluginServicesScope 记录注册的命令、owner 销毁连接和消息订阅连接。

owner 销毁时注销对应命令；整个 scope 清理时，先断开订阅，再逐个注销自己记录的命令。注销前还会核对总线上是否仍是原来的处理器，避免移除后来替换的对象。

设备插件初始化只注册成功第一条命令时，后续失败也能通过这个作用域撤销已有注册。不会要求业务插件逐项记住并清理全部主程序内部细节。

## 9. 为什么新面板一打开就能显示设备状态

[DeviceStatusBinding.cpp](C:/Users/35536/Desktop/LearnFramework/LearnFramework/examples/ContentPlugin/widgets/DeviceStatusBinding.cpp) 中的 observeDevice 顺序是：

1. 建立标签。
2. 订阅 StatusChanged。
3. 发送 QueryStatus。
4. 设备插件广播当前快照。

必须先订阅再查询，否则同步返回的广播可能发生在订阅建立之前。

QueryStatus 的回复仍然是广播，不是私有回复。新建第二个面板会让第一个面板也再收到一次状态，因此标签里的消息序号是“这个标签收到的通知次数”，不是设备操作次数，更不是全局唯一事件编号。

## 10. 返回值和线程限制

sendCommand 返回 false 可能表示编号不允许、线程不符合、无处理器或业务处理失败，没有细分错误码。

publish 返回 true 只表示通过检查并完成发布，不代表有订阅者，更不代表业务接收成功。

接口限定当前 GUI 线程，编号要大于等于 2000。后台线程不能直接据此更新窗口；长时间的硬件操作如果塞进当前命令回调，会阻塞界面。现有实现尚未提供后台任务调度。

本章自检：为什么输入 GWidget 的文字不会触发设备消息？因为那条连接直接从 QLineEdit::textChanged 到 QLabel::setText，完全不经过插件总线。

源码入口：[通信适配实现](C:/Users/35536/Desktop/LearnFramework/LearnFramework/src/runtime/PluginMessaging.cpp)、[服务接口](C:/Users/35536/Desktop/LearnFramework/LearnFramework/src/foundation/PluginServices.h)、[设备命令处理](C:/Users/35536/Desktop/LearnFramework/LearnFramework/examples/DevicePlugin/DevicePlugin.cpp)。

