#pragma once

// 利用构造/析构配对管理命令绑定；自身不直接拥有处理器指针。
class CommandBindings
{
public:
    CommandBindings();
    ~CommandBindings();
};
