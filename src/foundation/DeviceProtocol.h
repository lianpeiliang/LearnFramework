#pragma once

// ContentPlugin 和 DevicePlugin 只共享消息约定，不包含彼此的业务类。
namespace DeviceProtocol
{
enum : unsigned int
{
    SetConnection = 2000, // 命令：{ connected: bool }
    QueryStatus = 2001,   // 命令：无载荷；设备以 StatusChanged 消息回复当前状态
    StatusChanged = 3000 // 消息：{ connected: bool, name: QString, refreshInterval: int }
};
}
