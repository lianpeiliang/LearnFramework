# 04 UI、容器与窗口工厂

[返回目录](README.md) · [上一章](03-startup.md) · [下一章](05-plugins.md)

## 1. 一个区域其实由三层组成

以左上区域为例：

```text
QGroupBox：显示“内容 · 计数”标题
  └─ WidgetContainer：通用的承载外壳
       └─ FWidget：真正的计数与通信面板
```

外层布局和分组标题来自 UI，容器来自主程序，FWidget 来自 ContentPlugin。这样容器可以换内容，而外部布局保持原来的结构。

## 2. 逐项读 UI 片段

[main.ui](C:/Users/35536/Desktop/LearnFramework/LearnFramework/ui/main.ui) 中的源码片段：

```xml
<widget class="WidgetContainer" name="contentCounterContainer">
 <property name="contentClass" stdset="0">
  <string>plugin.f</string>
 </property>
</widget>
```

- class 告诉 UI 加载器创建哪种外壳：WidgetContainer。
- name 成为 objectName，帮助代码或测试找到这个具体容器。
- contentClass 是动态属性，保存工厂配置键。
- plugin.f 不要求等于 C++ 类名，代码最终把它映射到 FWidget。
- stdset="0" 配合这里的写法保存动态属性，不能理解成 C++ setter 的调用。

Designer 的“提升为 WidgetContainer”只是在 UI 中声明这个类名。运行时真正创建它的能力来自 RuntimeUiLoader 的重写函数。

## 3. 为什么 UI 加载要分成两遍

[RuntimeUiLoader.cpp](C:/Users/35536/Desktop/LearnFramework/LearnFramework/src/widgets/RuntimeUiLoader.cpp) 重写 createWidget：

```text
发现 WidgetContainer → new WidgetContainer(factory, parent)
其他类名 → 交给 QUiLoader 标准流程
```

刚执行 createWidget 时，UI 属性还没有全部赋好，因此不能马上按 contentClass 创建内容。等 load 完成后，再执行 populateContainers。

populateContainers 递归查找后代容器，另外检查根窗口自身是否也是容器，然后依次调用：

```cpp
container->setContentClass(
    container->property("contentClass").toString(), error);
```

遇到第一个失败会返回 false。函数本身不回滚之前的成功填充；ApplicationHost 通过销毁整个根窗口清理已创建对象。这里要分清“局部函数的行为”和“上层整体失败处理”。

## 4. 工厂是一本创建方法字典

[WidgetFactory](C:/Users/35536/Desktop/LearnFramework/LearnFramework/src/widgets/WidgetFactory.h) 保存：

```text
"c"              → 创建 CWidget 的函数
"plugin.f"       → 经 ContentPlugin 创建 FWidget 的函数
"device.monitor" → 经 DevicePlugin 创建设备监控的函数
```

registerWidget 做的事是检查名称和函数，再保存函数。create 做的事是查找函数并执行。

它不保存创建好的面板，也不缓存上一次返回的窗口。因此两个容器请求 c 会得到两个不同对象。

登记拒绝纯空白名称、空函数和重复名称。合法名称按原样保存，大小写和首尾空格会影响查找；不要把它当作自动规范化的名称系统。

## 5. 内置和插件为何能走同一条路

内置回调直接 `new CWidget(parent)`。插件回调则调用：

```text
LoadedPlugin::createWidget
  → WidgetProvider::createWidget
    → ContentPlugin::createWidget
      → new FWidget(services, parent)
```

它们都满足“传入 QWidget* parent，返回 QWidget*”的函数形状。容器只处理这个统一入口，不需要判断内容究竟来自主程序还是 DLL。

这是工厂模式在本项目中解决的具体问题：把选择实现类的判断从容器代码里移走。

## 6. setContentClass 如何保证替换失败不丢掉旧内容

[WidgetContainer.cpp](C:/Users/35536/Desktop/LearnFramework/LearnFramework/src/widgets/WidgetContainer.cpp) 的逻辑：

1. 清空错误信息。
2. 当前已有相同类型时，直接成功，保留已有对象和状态。
3. 名称非空时先尝试创建新内容。
4. 创建失败则返回 false，旧内容仍在。
5. 创建成功后，移除并删除旧内容。
6. 更新 content、loadedClass 和动态属性。
7. 把新内容放进布局并显示。

空字符串表示主动清空。再次设置相同类型不会重建，这解释了为什么同类型设置不重置计数器。

这个保证仅针对“旧内容保留”：新内容的构造函数可能已经进行了查询等外部操作。例如创建 FWidget 时会请求设备状态，并广播给其他订阅者，因此不能把整个构造过程当成没有副作用的数据库事务。

## 7. 工厂副本是什么意思

RuntimeUiLoader 借用原工厂；WidgetContainer 保存工厂值副本。每个副本持有当时登记的创建函数。

副本不是实时共享的注册表：后来在原工厂添加名称，不会自动添加到已有容器的副本。但函数捕获的 shared_ptr 仍可指向同一个插件模块，因此所有副本共同维持模块寿命。

这也是为什么“复制了工厂”不等于“复制了 DLL 或所有窗口”。

## 8. 修改界面的正确方式

在运行目录的 ui/main.ui 中修改 contentClass，保存并重启，适用于选择已经登记的类型。

在 C++ 中运行时切换，必须调用 setContentClass。例如教学片段：

```cpp
auto *container = root->findChild<WidgetContainer *>("contentCounterContainer");
QString error;
if (container && !container->setContentClass("d", &error))
    qWarning().noquote() << error;
```

其中 root 必须是有效主窗口指针；这是讲解片段，不能直接粘到任何函数里。单纯 setProperty 不会触发切换，因为这里没有把 contentClass 定义为带 setter 的 Q_PROPERTY，也没有实现属性变更监听。

运行时切换不写回磁盘。重启后的内容仍由 UI 文件决定。

## 9. 本章自检

为什么把 plugin.f 改成不存在的名字，程序启动会失败，而运行时 setContentClass 失败可能保留界面？因为启动宿主遇到填充失败会销毁整个根窗口；单个容器的替换函数则先创建后删除，失败时不动旧内容。

