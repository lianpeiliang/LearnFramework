#pragma once

// ContentPlugin 和 DevicePlugin 只共享消息约定，不包含彼此的业务类。
// 编号区分操作，下面字段通过 PluginServices 的 QVariantMap 载荷传递。
namespace DeviceProtocol
{
enum : unsigned int
{
    SetConnection = 2000, // 命令：{ connected: bool }，设置后发布 StatusChanged
    QueryStatus = 2001,   // 命令：无载荷；设备以 StatusChanged 消息回复当前状态
    StatusChanged = 3000 // 消息：{ connected: bool, name: QString, refreshInterval: int（毫秒） }
};
}
