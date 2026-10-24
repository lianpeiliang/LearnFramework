#pragma once

class WidgetFactory;

// 利用构造/析构配对管理命令绑定；自身不直接拥有处理器指针。
class CommandBindings
{
public:
    explicit CommandBindings(WidgetFactory *factory = nullptr);
    ~CommandBindings();
    bool isRegistered() const { return registered; }
    CommandBindings(const CommandBindings &) = delete;
    CommandBindings &operator=(const CommandBindings &) = delete;

private:
    bool registered = false;
};
