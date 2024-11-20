#include <muduo/net/TcpServer.h>
#include <muduo/net/EventLoop.h>
#include <iostream>
#include <functional>
using namespace std;
using namespace muduo::net;
/*
基于muduo网络库开发服务器程序
1.组合TcpServer对象
2.创建EventLoop事件循环对象的指针
3.明确TcpServer构造函数需要什么参数，输出ChatServer的构造函数
4.在当前服务器类的构造函数当中，注册处理连接的回调函数，和处理读写事件的回调函数
5.设置合适的服务端线程数量，muduo会自己划分I/O线程和worker线程
*/
class chatServer
{
public:
    chatServer(EventLoop *loop, //事件循环
    const InetAddress &listenAddr,
    const string &nameArg)  //服务器的名字
    :_server(loop,listenAddr,nameArg),
    _loop(loop)
    {
        //给服务器注册用户连接的创建和断开回调
        _server.setConnectionCallback(bind(&chatServer::onConnetcion,this,placeholders::_1));
        //给服务器注册用户读写事件回调
        _server.setMessageCallback(bind(&chatServer::onMessage,this,placeholders::_1,placeholders::_2,placeholders::_3));
        //设置服务器端端线程数量
        _server.setThreadNum(8);

    }
    //开启事件循环  
    void start()
    {
        _server.start();
    }
private:
    TcpServer _server; // #1
    EventLoop *_loop; //#2 epoll

    void onConnetcion(const TcpConnectionPtr &ptr)
    {

        if(ptr->connected())
        {
            cout<<ptr->peerAddress().toIpPort()<<"->"<<ptr->localAddress().toIpPort()<<"\tstate:online!\n";

        }
        else
        {
            cout<<ptr->peerAddress().toIpPort()<<"->"<<ptr->localAddress().toIpPort()<<"\tstate:outline!\n";
            ptr->shutdown();
            _loop->quit();
        }

    }

    void onMessage(const TcpConnectionPtr &ptr,Buffer *buf,muduo::Timestamp stamp)
    {
        string strBuf = buf->retrieveAllAsString();
        cout<<"rece data:"<<strBuf<<" time:"<<stamp.toFormattedString()<<endl;
        ptr->send(strBuf);
    }
};

int main()
{
    EventLoop loop; //epoll
    InetAddress addr("127.0.0.1",6000);
    chatServer server(&loop,addr,"chatServer");

    server.start(); //listenfd epoll_ctl=>epoll
    loop.loop();    //epoll_wait以阻塞方式等待新用户连接，已连接用户的读写事件等

    return 0;
}