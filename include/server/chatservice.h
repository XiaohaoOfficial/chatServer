#ifndef CHATSERVICE_H
#define CHATSERVICE_H

#include <unordered_map>
#include <functional>
#include <muduo/net/TcpConnection.h>
#include "json.hpp"
#include "usermodel.h"
#include "friendmodel.h"
#include "offlinemessagemodel.h"
#include "groupmodel.h"
#include "redis.h"
#include <mutex>
using namespace std;
using namespace muduo::net;
using namespace muduo;
using json = nlohmann::json;
//处理事件的回调操作类型
using MsgHandler  = function<void(const TcpConnectionPtr &conn,json js,Timestamp)>;
//单例模式
class ChatService
{
public: 
    static ChatService* getInstance();
    //处理登陆业务
    void login(const TcpConnectionPtr &conn,json js,Timestamp);
    //处理注册业务
    void reg(const TcpConnectionPtr &conn,json js,Timestamp);
    //处理客户端退出
    void clientCloseException(const TcpConnectionPtr &conn);
    //一对一聊天业务
    void oneChat(const TcpConnectionPtr &conn,json js,Timestamp);
    //添加好友
    void addFriend(const TcpConnectionPtr &conn,json js,Timestamp);
    //服务器异常退出后，业务重置方法
    void resetState();
    //客户退出方法
    void loginout(const TcpConnectionPtr &conn, json js, Timestamp time);
    void unSubscribeRedis();

    MsgHandler getHandler(int msgId);
private:
    ChatService();
    //存储消息id和其对应的业务处理方法
    unordered_map<int,MsgHandler> _msgHandlerMap;

    //数据操作类对象
    UserModel _userModel;
    //信息操作类对象
    OfflineMsgModel _offlineMsgModel;
    //好友操作类对象
    FriendModel _friendModel;
    //群组操作类对象
    GroupModel _groupModel;
    //存储在线用户的通信连接
    unordered_map<int,TcpConnectionPtr> _userConnnectionMap;
    //Redis对象，用于实现跨服务器通信
    Redis _redis;
    // 创建群组业务
    void createGroup(const TcpConnectionPtr &conn, json js, Timestamp time);
    // 加入群组业务
    void addGroup(const TcpConnectionPtr &conn, json js, Timestamp time);
    // 群组聊天业务
    void groupChat(const TcpConnectionPtr &conn, json js, Timestamp time);

    mutex _mtx;
};



#endif