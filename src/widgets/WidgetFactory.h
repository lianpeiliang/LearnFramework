#pragma once

#include <QHash>
#include <QString>
#include <functional>

class QWidget;

// 将 UI 中使用的内容名称映射到创建回调，工厂本身不保存或拥有窗口实例。
// 回调应创建独立实例并使用传入的 parent，由 Qt 父子对象机制管理窗口生命周期。
class WidgetFactory
{
public:
    using Creator = std::function<QWidget *(QWidget *parent)>;
    // 不允许纯空白名称、空回调和同名覆盖；合法名称按原样保存，匹配时区分大小写。
    bool registerWidget(const QString &name, Creator creator);
    bool contains(const QString &name) const;
    // 释放本工厂保存的回调；若因此释放插件的最后一个模块引用，会触发插件窗口清理。
    void clear();
    // 名称未登记或回调创建失败时返回 nullptr，不缓存创建结果。
    QWidget *create(const QString &name, QWidget *parent) const;

private:
    // 工厂可按值复制；复制后各自登记的名称互不影响，回调捕获的共享资源仍可共享。
    QHash<QString, Creator> creators;
};
