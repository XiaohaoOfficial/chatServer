#ifndef CHATSERVER_H
#define CHATSERVER_H

#include <muduo/net/TcpServer.h>
#include <muduo/net/EventLoop.h>
using namespace muduo;
using namespace muduo::net;

//聊天服务器的主类
class ChatServer
{
public:
    ChatServer(EventLoop *loop,const InetAddress &addr,const string nameArg);

    void start();
private:
    void onConnetcion(const TcpConnectionPtr &conn);

    void onMessage(const TcpConnectionPtr &conn,Buffer *buf,Timestamp stamp);

    //组合的muduo库，实现服务器功能的类对象
    TcpServer _server;
    //指向事件循环对象的指针
    EventLoop *_loop;

};




#endif