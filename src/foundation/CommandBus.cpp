#include "CommandBus.h"
#include<QDebug>

CommandBus::CommandBus()
{
}

CommandBus::~CommandBus()
{
    // 只清理总线拥有的命令处理器；纯消息订阅者的生命周期仍由外部负责。
    clearHandlers();
}

// 注册指定命令处理器
int CommandBus::registerHandler(unsigned int commandId, CommandHandler * handler)
{
    if (findHandler(commandId) != NULL)
    {
        // 同一CmdID不能重复注册
        return -1;
    }
    // 同一个处理器可以绑定多个命令，每个成功绑定都占一份引用。
    commandHandlers[commandId] = handler;
    retainHandler(handler);
    return 0;
}

// 注册指定命令范围的处理器
int CommandBus::registerHandler(unsigned int firstCommand, unsigned int lastCommand, CommandHandler *handler)
{
    for (unsigned int commandId = firstCommand ; commandId <= lastCommand ; commandId++)
    {
        if (registerHandler(commandId, handler) == -1)
        {
            // 回滚前面成功的绑定；若这些是 handler 的全部引用，回滚也会销毁它。
            // commandId 为 0 时减一会下溢，调用者需避开这一边界情形。
            unregisterHandler(firstCommand, commandId-1);
            return -1;
        }
    }
    return 0;
}

// 注销指定命令处理器
void CommandBus::unregisterHandler(unsigned int commandId)
{
    CommandHandler * handler = findHandler(commandId);
    if (handler == NULL)
    {
        return;
    }
    // 从map中删除
    commandHandlers.erase(commandId);

    // 最后一份命令引用释放时会销毁处理器，此后不能继续访问 handler。
    releaseHandler(handler);
}

// 注销指定命令范围处理器
void CommandBus::unregisterHandler(unsigned int firstCommand, unsigned int lastCommand)
{
    for (unsigned int commandId = firstCommand ; commandId <= lastCommand ; commandId++)
    {
        unregisterHandler(commandId);
    }
}

// 命令分发
int CommandBus::dispatchCommand(unsigned int commandId, char * data /*= 0L*/, int length/* = 0*/)
{
    CommandHandler * handler = findHandler(commandId);
    if (handler == 0L)
    {
        return 0;
    }
    // 虚函数调用进入具体处理器；总线不需要知道它是不是插件加载器。
    return handler->dispatchCommand(commandId, data, length);
}

int CommandBus::subscribe(unsigned int messageId, CommandHandler *handler)
{
    // 只保存借用指针；重复插入会保留多份，调用方必须保证 handler 非空且有效。
    messageSubscribers.insert(messageId,handler);
    return 1;
}

void CommandBus::unsubscribe(unsigned int messageId)
{
    messageSubscribers.remove(messageId);
}

void CommandBus::unsubscribe(unsigned int messageId, CommandHandler *handler)
{
    messageSubscribers.remove(messageId,handler);
}

int CommandBus::dispatchMessage(unsigned int messageId, void *data, int length)
{
    // values() 返回本轮订阅快照，回调中增删原表不会改变这次遍历的内容。
    // 快照只复制指针，不延长对象寿命；不能销毁本轮尚未通知的其他订阅者。
    QList<CommandHandler *> listHandler = messageSubscribers.values(messageId);
    for(int i = 0;i<listHandler.count();i++)
    {
        CommandHandler *handler = listHandler.at(i);
        handler->dispatchMessage(messageId,data,length);
    }
    return 1;
}

// 删除所有处理器
void CommandBus::clearHandlers()
{
    // 按命令条目逐一减引用，而非对每个条目直接 delete，避免共享处理器重复释放。
    for (HandlerTable::iterator iter = commandHandlers.begin() ; iter != commandHandlers.end() ; iter++)
    {
        qDebug()<<"---------------"<<(*iter).first;         //打印ID号
        // 删除处理器
        releaseHandler((*iter).second);
    }
    commandHandlers.clear();
}


// 查找处理器
CommandHandler * CommandBus::findHandler(unsigned int commandId)
{
    HandlerTable::iterator iter = commandHandlers.find(commandId);
    if (iter == commandHandlers.end())
    {
        return 0L;
    }
    else
    {
        return (*iter).second;
    }
}


// 只统计成功的命令绑定；消息订阅和外部持有的指针均不增加计数。
void CommandBus::retainHandler(CommandHandler * handler)
{
    handlerReferences[handler] = referenceCount(handler) + 1;
}

// 删除Handler引用， 当引用为0时，删除该Handler
void CommandBus::releaseHandler(CommandHandler * handler)
{
    int nRef = referenceCount(handler) -1;
    if (nRef == 0)
    {
        handlerReferences.erase(handler);
        // 通过虚函数释放，允许派生处理器在析构中关闭插件等资源。
        // 注意：消息订阅表不会自动清理，调用方必须事先取消相关订阅。
        handler->dispose();
        handler = NULL;
    }
    else if (nRef > 0)
    {
        handlerReferences[handler] = nRef;
    }
}

// 获得引用值
int CommandBus::referenceCount(CommandHandler * handler)
{
    HandlerRefCounts::iterator iter = handlerReferences.find(handler);
    if (iter == handlerReferences.end())
    {
        return 0;
    }
    else
    {
        return (*iter).second;

    }
}

// 查找处理器
CommandHandler * CommandBus::handlerFor(unsigned int commandId)
{
    return findHandler(commandId);
}

