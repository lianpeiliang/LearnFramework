#pragma once

#include <map>
#include<QMap>
using namespace std;
/*
类名:		CommandHandler
功能说明:	命令处理器接口
*/
// 命令与消息统一使用这一接口，处理器通过编号区分具体操作。
// data/length 是借用的字节缓冲区；分发器不复制数据，也不延长其生命周期。
class  CommandHandler
{
public:
    virtual ~CommandHandler()
    {
    }

public:
    //  命令执行
    virtual bool dispatchCommand(unsigned int commandId, char * data = 0L, int length = 0)=0;
    //消息执行，包括发送和接收
    virtual bool dispatchMessage(unsigned int messageId,void * data = NULL, int length = 0) = 0;
    // 删除处理器，用于不同模块删除处理器本身
    virtual void dispose()
    {
        // 会销毁自身，因此通过命令绑定交给总线管理的处理器应在堆上创建。
        delete this;
    }
};



/*
类名:		CommandBus
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
    int registerHandler(unsigned int commandId, CommandHandler * handler);

    // 注册闭区间内的命令；调用者应保证 firstCommand <= lastCommand。
    // 当前回滚和无符号边界处理仍有局限，区间首项冲突等情形需后续完善。
    int registerHandler(unsigned int firstCommand, unsigned int lastCommand, CommandHandler *handler);

    // 注销指定命令处理器
    void unregisterHandler(unsigned int commandId);

    // 注销指定命令范围处理器
    void unregisterHandler(unsigned int firstCommand, unsigned int lastCommand);

    // 同步执行命令，返回处理器的 bool 结果（转为 int）。未找到也返回 0。
    int dispatchCommand(unsigned int commandId, char * data = 0L, int length = 0);

    // 订阅消息，不增加处理器引用计数；对象销毁前须主动取消订阅。
    // 当前允许同一处理器重复订阅，因此可能收到重复通知。
    int subscribe(unsigned int messageId,CommandHandler * handler);
    //注销消息号，整个注销在这个ID下的所有的Handler
    void unsubscribe(unsigned int messageId);
    //只是注销相应对应的Handler和其消息号码
    void unsubscribe(unsigned int messageId,CommandHandler * handler);
    // 同步通知所有订阅者；当前固定返回 1，不汇总各处理器的执行结果。
    int dispatchMessage(unsigned int messageId,void * data = NULL, int length = 0);


    // 查询借用指针；解绑后指针可能失效，调用者不能自行删除。
    CommandHandler * handlerFor(unsigned int commandId);
protected:
    // 删除所有处理器
    void clearHandlers();

    // 查找处理器
    CommandHandler * findHandler(unsigned int commandId);

    // 增加Handler引用
    void retainHandler(CommandHandler * handler);

    // 删除Handler引用， 当引用为0时，删除该Handler
    void releaseHandler(CommandHandler * handler);

    // 获得引用值
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
