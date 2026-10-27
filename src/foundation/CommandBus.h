#pragma once

#include <map>
#include<QMap>
using namespace std;
/*
类名:		CommandHandler
功能说明:	命令处理器接口
*/
// 命令与消息统一使用这一接口，处理器通过编号区分具体操作。
// data/length 是借用的载荷及其长度；分发器不复制数据，也不延长其生命周期。
// 载荷的具体类型、长度单位及空指针含义由发送方与处理器约定。
class  CommandHandler
{
public:
    virtual ~CommandHandler()
    {
    }

public:
    // 命令执行；返回值由具体处理器定义，通常表示是否处理成功。
    virtual bool dispatchCommand(unsigned int commandId, char * data = 0L, int length = 0)=0;
    // 消息回调；CommandBus 当前不会收集或传播此返回值。
    virtual bool dispatchMessage(unsigned int messageId,void * data = NULL, int length = 0) = 0;
    // 删除处理器，用于不同模块删除处理器本身
    virtual void dispose()
    {
        // 会销毁自身，因此通过命令绑定交给总线管理的处理器应在堆上创建。
        delete this;
    }
};



/*
类名:CommandBus
功能说明:	命令分发管理类
*/
// 命令：一个编号对应一个处理器；消息：一个编号可以对应多个订阅者。
// 当前实现同步调用处理器，不排队、不切换线程，也不提供线程安全保证。
class  CommandBus
{
public:
    CommandBus();
    virtual ~CommandBus();

public:
    // 注册指定命令处理器：成功返回 0，编号冲突返回 -1。
    // 注册成功才增加引用计数；注册失败时，新传入对象仍由调用方管理。
    // handler 必须非空，成功后由总线在最后一个命令绑定解除时调用 dispose。
    int registerHandler(unsigned int commandId, CommandHandler * handler);

    // 注册闭区间内的命令；发生冲突时尝试撤销本次已成功注册的编号。
    // 调用方应保证 firstCommand <= lastCommand，倒置区间会不注册任何命令并返回成功。
    // 部分成功后的回滚可能释放 handler，不能一概认为失败后对象仍然存活。
    // 当前未检查无符号边界：lastCommand 不能为最大值；首项为 0 且冲突时回滚会下溢。
    int registerHandler(unsigned int firstCommand, unsigned int lastCommand, CommandHandler *handler);

    // 解除一个命令绑定；不存在的编号不做处理，最后一个绑定解除时销毁处理器。
    void unregisterHandler(unsigned int commandId);

    // 解除闭区间内的绑定；同样要求 lastCommand 小于 unsigned int 的最大值。
    void unregisterHandler(unsigned int firstCommand, unsigned int lastCommand);

    // 同步执行命令，返回处理器的 bool 结果（转为 int）。未找到也返回 0。
    int dispatchCommand(unsigned int commandId, char * data = 0L, int length = 0);

    // 订阅消息，不增加处理器引用计数；对象销毁前须主动取消订阅。
    // 当前允许同一处理器重复订阅，因此可能收到重复通知。
    int subscribe(unsigned int messageId,CommandHandler * handler);
    // 移除该消息编号的全部订阅，不销毁订阅者。
    void unsubscribe(unsigned int messageId);
    // 移除该编号下指定处理器的全部重复订阅，不影响其命令绑定。
    void unsubscribe(unsigned int messageId,CommandHandler * handler);
    // 同步遍历调用开始时的订阅快照；回调中的订阅变化留到下次分发生效。
    // 当前固定返回 1，即使没有订阅者，也不汇总各处理器的执行结果。
    int dispatchMessage(unsigned int messageId,void * data = NULL, int length = 0);


    // 查询借用指针；解绑后指针可能失效，调用者不能自行删除。
    CommandHandler * handlerFor(unsigned int commandId);
protected:
    // 释放全部命令绑定；消息订阅不参与引用计数，也不会在这里逐项取消。
    void clearHandlers();

    // 查找处理器
    CommandHandler * findHandler(unsigned int commandId);

    // 增加Handler引用
    void retainHandler(CommandHandler * handler);

    // 删除Handler引用， 当引用为0时，删除该Handler
    void releaseHandler(CommandHandler * handler);

    // 查询命令绑定数量；未记录的处理器返回 0，不会因此创建计数条目。
    int referenceCount(CommandHandler * handler);


protected:
    typedef map<CommandHandler *, int> HandlerRefCounts;
    typedef map<unsigned int, CommandHandler *> HandlerTable;

    // 记录每个处理器绑定了多少个命令，不包含消息订阅数量。
    HandlerRefCounts handlerReferences;

    // 命令编号到处理器的唯一映射。
    HandlerTable commandHandlers;
    // 一个消息编号可映射到多个处理器，这些指针不拥有对象。
    QMultiMap<unsigned int,CommandHandler *> messageSubscribers;

    typedef QMap<QString, void *> ServiceTable;
};
