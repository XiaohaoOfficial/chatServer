#include "chatserver.h"
#include "json.hpp"
#include <functional>
#include <string>
#include "chatservice.h"
using namespace std;
using json = nlohmann::json;
ChatServer::ChatServer(EventLoop *loop,const InetAddress &addr,const string nameArg)
:_loop(loop),_server(loop,addr,nameArg)
{
    //注册连接回调
    _server.setConnectionCallback(bind(&ChatServer::onConnetcion,this,placeholders::_1));

    //注册消息回调
    _server.setMessageCallback(bind(&ChatServer::onMessage,this,placeholders::_1,placeholders::_2,placeholders::_3));

    //设置线程数量
    _server.setThreadNum(4);
    
}

void ChatServer::start()
{
    _server.start();
}

void ChatServer::onConnetcion(const TcpConnectionPtr &conn)
{
    //客户端断开连接
    if(!conn->connected())
    {
        ChatService::getInstance()->clientCloseException(conn);
        conn->shutdown();
    }
}

void ChatServer::onMessage(const TcpConnectionPtr &conn,Buffer *buf,Timestamp stamp)
{
    string strBuf = buf->retrieveAllAsString();
    //数据的反序列化
    //达到的目的：完全解耦网络模块的代码和业务模块的代码
    json js = json::parse(strBuf);
    auto handler = ChatService::getInstance()->getHandler(js["msgId"].get<int>());
    handler(conn,js,stamp);
}
