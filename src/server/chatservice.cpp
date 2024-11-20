#include "chatservice.h"
#include "public.h"
#include <muduo/base/Logging.h>
ChatService* ChatService::getInstance()
{
    static ChatService service;
    return &service;
}

ChatService::ChatService()
{
    _msgHandlerMap.insert({LOGIN_MSG,bind(&ChatService::login,this,placeholders::_1,placeholders::_2,placeholders::_3)});
    _msgHandlerMap.insert({REG_MSG,bind(&ChatService::reg,this,placeholders::_1,placeholders::_2,placeholders::_3)});
    _msgHandlerMap.insert({ONE_CHAT_MSG,bind(&ChatService::oneChat,this,placeholders::_1,placeholders::_2,placeholders::_3)});
    _msgHandlerMap.insert({ADD_FRIEND_MSG,bind(&ChatService::addFriend,this,placeholders::_1,placeholders::_2,placeholders::_3)});
    // 群组业务管理相关事件处理回调注册
    _msgHandlerMap.insert({CREATE_GROUP_MSG, bind(&ChatService::createGroup, this, placeholders::_1, placeholders::_2, placeholders::_3)});
    _msgHandlerMap.insert({ADD_GROUP_MSG, bind(&ChatService::addGroup, this, placeholders::_1, placeholders::_2, placeholders::_3)});
    _msgHandlerMap.insert({GROUP_CHAT_MSG, bind(&ChatService::groupChat, this, placeholders::_1, placeholders::_2, placeholders::_3)});
    _msgHandlerMap.insert({LOGINOUT_MSG, bind(&ChatService::loginout, this, placeholders::_1, placeholders::_2, placeholders::_3)});
                    LOG_INFO<<"test";

    _redis.connect();
                    LOG_INFO<<"test";

    _redis.init_notify_handler([&](int channel,string msg){
        auto it = _userConnnectionMap.find(channel);
        if(it != _userConnnectionMap.end())
        {
            it->second->send(msg);
        }
    });
                LOG_INFO<<"test";

}

MsgHandler ChatService::getHandler(int msgId)
{
    if(_msgHandlerMap.find(msgId) == _msgHandlerMap.end())
    {
        return [=](const TcpConnectionPtr &conn,json js,Timestamp)->void
        {
            LOG_ERROR<<"msg_Id:"<<msgId<<"   can't find handler!";
        };
    }
    return _msgHandlerMap[msgId];
}
//  处理登陆业务  ORM Object Relation Map 业务层操作的都是对象 DAO
void ChatService::login(const TcpConnectionPtr &conn,json js,Timestamp)
{
    LOG_INFO<<"do login service";
    int id = js["id"];
    string password = js["password"];
    User user = _userModel.query(id);
    if(user.getId() == -1)
    {
        LOG_ERROR<<"获取用户失败！";
        return;
    }
    json response;
    response["msgId"]=LOGIN_MSG_ACK;
    if(password == user.getPassword())
    {
        //登录成功
       if(user.getState() == "OFFLINE")
       {
            {
                lock_guard<mutex> lck(_mtx);
                _userConnnectionMap.insert({id,conn});
            }
            response["errno"]=0;
            response["id"]=user.getId();
            response["name"]=user.getName();
            user.setSTATE("ONLINE");
            _userModel.updateState(user,ONLINE);
            //查询用户是否有离线信息，有的话，返回信息
            auto msgVec = _offlineMsgModel.query(id);

            //用户登陆成功，监听该用户id对应的Redischannel
            _redis.subscribe(id);
            LOG_INFO<<"_redis->subscribe";
            if(!msgVec.empty())
            {
                json resp;
                vector<string> jsonVec;
                for(string str : msgVec)
                {
                    jsonVec.push_back(str);
                }
                resp["jsonVec"] = jsonVec;
                resp["msgId"]= JSON_VEC_MSG;
                conn->send(resp.dump());
                jsonVec.clear();
                _offlineMsgModel.remove(id);
            }
            
            //查询好友信息并返回
            auto friendVec =  _friendModel.query(id);
            
            if(!friendVec.empty())
            {
                vector<string> friendInfo;
                for(auto frd : friendVec)
                {
                    json js;
                    js["name"] = frd.getName();
                    js["id"]=frd.getId();
                    js["state"]=frd.getState();
                    friendInfo.push_back(js.dump());
                }
                response["friends"] = friendInfo;
            }
       }
       else
       {
            response["errMsg"]="该用户已登陆";
            response["errno"]=2;
       }
    }
    else
    {
        response["errMsg"]="密码错误";
        response["errno"]=1;
    }
    LOG_INFO<<response.dump();
    conn->send(response.dump());

}
//处理注册业务 name password
void ChatService::reg(const TcpConnectionPtr &conn,json js,Timestamp)
{
    string name = js["name"];
    string password = js["password"];
    User user;
    user.setName(name);
    user.setPassword(password);
    bool noErr =  _userModel.insert(user);
    if(noErr)   //注册成功
    {
        json response;
        response["msgId"] = REG_MSG_ACK;
        response["id"]=user.getId();
        response["errno"]=0;
        conn->send(response.dump());
    }
    else    //注册失败
    {   
        json response;
        response["msgId"] = REG_MSG_ACK;
        response["errno"]=1;
        conn->send(response.dump());
    }

    LOG_INFO<<"do reg service";

}

void ChatService::clientCloseException(const TcpConnectionPtr &conn)
{
    User user;
    {
        lock_guard<mutex> lck(_mtx);
        for(auto it = _userConnnectionMap.begin();it != _userConnnectionMap.end(); ++it)
        {
            if(it->second == conn)
            {
            _userConnnectionMap.erase(it);
            user.setId(it->first);
            break;
            }
        }
    }
    if(user.getId() != -1)
    {
        user.setSTATE("offline");
        _userModel.updateState(user,OFFLINE);
    }

}
void ChatService::oneChat(const TcpConnectionPtr &conn,json js,Timestamp time)
{
    int toId = js["to"].get<int>();
    string msg = js["message"];
    bool userIsExist = false;
    userIsExist = (_userModel.query(toId).getId() != -1);
    json toSender;
    toSender["msgId"]=ONE_CHAT_MSG;
    if(userIsExist)
    {
        lock_guard<mutex> lck(_mtx);
        auto it = _userConnnectionMap.find(toId);
        json resp;
        resp["message"] = msg;
        resp["msgId"]=RECEIVE_MSG;
        string fromName = _userModel.query(js["id"]).getName();
        resp["from"]= fromName;
        resp["errno"]=0;
        resp["time"]=time.toFormattedString();
        toSender["errno"]=0;
        //通信的双方在同一服务器上登陆，则直接发送
        if(it != _userConnnectionMap.end())
        {
            it->second->send(resp.dump());
            toSender["state"]="online";
            conn->send(toSender.dump());
            return;
        }
        else
        {
            auto toUser = _userModel.query(toId);
            //如果对方状态为离线，则将消息放入数据库中
            if(toUser.getState() == "OFFLINE")  
            {
                _offlineMsgModel.insert(toId,resp.dump());
                toSender["state"]="offline";
                conn->send(toSender.dump());
            }
            else    //如果对方状态为在线，则通过redis发送   
            {
                _redis.publish(toId,resp.dump());
                toSender["state"]="online";
                conn->send(toSender.dump());
            }
            
        }
    }
    else    //对方不存在
    {
        toSender["errno"]=1;
        conn->send(toSender.dump());
    }


}

void ChatService::resetState()
{
    for(auto it = _userConnnectionMap.begin();it != _userConnnectionMap.end();++it)
    {
        int userId = it->first;
        _userModel.updateState(userId,OFFLINE);
    }
}
void ChatService::addFriend(const TcpConnectionPtr &conn,json js,Timestamp)
{
    int friendId = js["friendId"];
    int userId = js["id"];
    json resp;
    resp["msgId"]=ADD_FRIEND_MSG;

    auto myFri = _friendModel.query(userId);
    for(auto fri: myFri)
    {
        if(fri.getId() == friendId)
        {
            resp["errno"]=1;
            conn->send(resp.dump());
            return;
        }
    }
    auto hisFri = _friendModel.query(friendId);
    for(auto user : hisFri)
    {
        if(userId == user.getId())
        {
            resp["errno"]=1;
            conn->send(resp.dump());
            return;
        }
    }
    _friendModel.insert(userId,friendId);
    
    resp["errno"]=0;
    conn->send(resp.dump());
}

// 创建群组业务
void ChatService::createGroup(const TcpConnectionPtr &conn, json js, Timestamp time)
{
    int userid = js["id"].get<int>();
    string name = js["groupName"];
    string desc = js["groupDesc"];
    
    json resp;
    // 存储新创建的群组信息
    Group group(-1, name, desc);
    if (_groupModel.createGroup(group))
    {
        // 存储群组创建人信息
        _groupModel.addGroup(userid, group.getId(), "creator");
        resp["errno"] = 0;
        resp["groupId"]= group.getId();
    }
    else
    {
        resp["errno"] = 1;
    }
    resp["msgId"]=CREATE_GROUP_MSG;
    conn->send(resp.dump());

}

// 加入群组业务
void ChatService::addGroup(const TcpConnectionPtr &conn, json js, Timestamp time)
{
    int userid = js["id"];
    int groupid = js["groupId"];
    auto groups = _groupModel.queryUserGroups(userid);
    json resp;
    resp["msgId"] = ADD_GROUP_MSG;
    for(auto group : groups)
    {
        if(group.getId() == groupid)
        {
            resp["errno"]=1;    //1代表已加入该群组
            conn->send(resp.dump());
            return;
        }
    }
    
    auto groupVec = _groupModel.queryGroups();
    bool groupExist = false;
    for(auto group : groupVec)
    {
        if(group.getId() == groupid)
        {
            groupExist = true;
        }
    }
    if(!groupExist)
    {
        resp["errno"] = 2; //2代表该群组不存在
        conn->send(resp.dump());
        return ;
    }
    _groupModel.addGroup(userid, groupid, "normal");
    
    resp["errno"] = 0;
    conn->send(resp.dump());
}

// 群组聊天业务
void ChatService::groupChat(const TcpConnectionPtr &conn, json js, Timestamp time)
{
    int userid = js["id"].get<int>();
    int groupid = js["groupId"].get<int>();
    vector<int> useridVec = _groupModel.queryGroupUsers(userid, groupid);
    json resp;
    resp["msgId"]=RECEIVE_GROUP_MSG;
    resp["from"]= _userModel.query(userid).getName();
    resp["groupId"] = groupid;
    resp["message"] = js["message"];
    resp["errno"]=0;
    resp["time"]=time.toFormattedString();
    lock_guard<mutex> lock(_mtx);
    for (int id : useridVec)
    {
        auto it = _userConnnectionMap.find(id);
        //如果对方和自己在同一个服务器，则直接发送
        if (it != _userConnnectionMap.end())
        {
            // 转发群消息
            it->second->send(resp.dump());
        }
        else
        {
            //如果对方离线，则插入数据库
            if(_userModel.query(id).getState() == "OFFLINE")
            {
                _offlineMsgModel.insert(id,resp.dump());
            }
            else    //如果对方在线，则将消息插入redis
            {
                _redis.publish(id,resp.dump());
            }
        }
    }
    json toSender;
    toSender["msgId"]=GROUP_CHAT_MSG;
    toSender["errno"]=0;
    conn->send(toSender.dump());
}
void ChatService::loginout(const TcpConnectionPtr &conn, json js, Timestamp time)
{
    json res;
    res["msgId"]=LOGINOUT_MSG;
    int id = js["id"];
    User user = _userModel.query(id);
    if(user.getId() != -1)
    {
        {
            lock_guard<mutex> lck(_mtx);
            auto it = _userConnnectionMap.find(id);
            _userConnnectionMap.erase(it);
        }
        _userModel.updateState(user,OFFLINE);
        res["errno"]=0;

        //用户退出登录成功，放弃监听该用户id对应的channel
        _redis.unsubscribe(id);

        conn->send(res.dump());
        return;
    }
    res["errno"]=1;
    conn->send(res.dump());
}
void ChatService::unSubscribeRedis()
{
    for(auto it = _userConnnectionMap.begin();it != _userConnnectionMap.end();++it)    
    {
        int id = it->first;
        _redis.unsubscribe(id);
    }
}
