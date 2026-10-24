#pragma once

class WidgetFactory;

// 利用构造/析构配对管理命令绑定；自身不直接拥有处理器指针。
class CommandBindings
{
public:
    explicit CommandBindings(WidgetFactory *factory = nullptr);
    ~CommandBindings();
    // 注册失败的绑定没有注销权，析构时不能移除其他宿主已注册的命令。
    bool isRegistered() const { return registered; }
    // 绑定只应有一个管理者，避免复制后多次注销同一命令。
    CommandBindings(const CommandBindings &) = delete;
    CommandBindings &operator=(const CommandBindings &) = delete;

private:
    bool registered = false;
};
