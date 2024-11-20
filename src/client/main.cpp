#include "json.hpp"
#include <iostream>
#include <thread>
#include <string>
#include <vector>
#include <chrono>
#include <ctime>
#include <unordered_map>
#include <functional>
using namespace std;
using json = nlohmann::json;

#include <unistd.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <semaphore.h>
#include <atomic>

#include "group.h"
#include "user.h"
#include "public.h"

// 记录当前系统登录的用户信息
User g_currentUser;
// 记录当前登录用户的好友列表信息
vector<User> g_currentUserFriendList;
// 记录当前登录用户的群组列表信息
vector<Group> g_currentUserGroupList;

// 控制主菜单页面程序
bool isMainMenuRunning = false;

// 用于读写线程之间的通信
sem_t rwsem;
// 记录登录状态
atomic_bool g_isLoginSuccess{false};


// 接收线程
void readTaskHandler(int clientfd);
// 获取系统时间（聊天信息需要添加时间信息）
string getCurrentTime();
// 主聊天页面程序
void mainMenu(int,int);
// 显示当前登录成功用户的基本信息
void showCurrentUserData();

// "help" command handler
void help(int fd = 0, string str = "",int id = -1);
// "chat" command handler
void chat(int, string,int);
// "addfriend" command handler
void addfriend(int, string,int);
// "creategroup" command handler
void creategroup(int, string,int);
// "addgroup" command handler
void addgroup(int, string,int);
// "groupchat" command handler
void groupchat(int, string,int);
// "loginout" command handler
void loginout(int, string,int);

// 系统支持的客户端命令列表
unordered_map<string, string> commandMap = 
{
    {"help", "显示所有支持的命令，格式help"},
    {"chat", "一对一聊天，格式chat:friendid:message"},
    {"addfriend", "添加好友，格式addfriend:friendid"},
    {"creategroup", "创建群组，格式creategroup:groupname:groupdesc"},
    {"addgroup", "加入群组，格式addgroup:groupid"},
    {"groupchat", "群聊，格式groupchat:groupid:message"},
    {"loginout", "注销，格式loginout"}
};

// 注册系统支持的客户端命令处理
unordered_map<string, function<void(int, string,int)>> commandHandlerMap = 
{
    {"help", help},
    {"chat", chat},
    {"addfriend", addfriend},
    {"creategroup", creategroup},
    {"addgroup", addgroup},
    {"groupchat", groupchat},
    {"loginout", loginout}
};

// 聊天客户端程序实现，main线程用作发送线程，子线程用作接收线程
int main(int argc, char **argv)
{
    // if (argc < 3)
    // {
    //     cerr << "command invalid! example: ./ChatClient 127.0.0.1 6000" << endl;
    //     exit(-1);
    // }

    // // 解析通过命令行参数传递的ip和port
    // char *ip = argv[1];
    // uint16_t port = atoi(argv[2]);

    // 创建client端的socket
    int clientfd = socket(AF_INET, SOCK_STREAM, 0);
    if (-1 == clientfd)
    {
        cerr << "socket create error" << endl;
        exit(-1);
    }

    // 填写client需要连接的server信息ip+port
    sockaddr_in server;
    memset(&server, 0, sizeof(sockaddr_in));

    server.sin_family = AF_INET;
    server.sin_port = htons(8000);
    server.sin_addr.s_addr = inet_addr("127.0.0.1");

    // client和server进行连接
    if (-1 == connect(clientfd, (sockaddr *)&server, sizeof(sockaddr_in)))
    {
        cerr << "connect server error" << endl;
        close(clientfd);
        exit(-1);
    }

    // 初始化读写线程通信用的信号量
    sem_init(&rwsem, 0, 0);

    // 连接服务器成功，启动接收子线程
    std::thread readTask(readTaskHandler, clientfd); // pthread_create
    readTask.detach();                               // pthread_detach

    // main线程用于接收用户输入，负责发送数据
    for (;;)
    {
        // 显示首页面菜单 登录、注册、退出
        cout << "========================" << endl;
        cout << "1. login" << endl;
        cout << "2. register" << endl;
        cout << "3. quit" << endl;
        cout << "========================" << endl;
        cout << "choice:";
        int choice = 0;
        cin >> choice;
        cin.get(); // 读掉缓冲区残留的回车

        switch (choice)
        {
        case 1: // login业务
        {
            int id = 0;
            char pwd[50] = {0};
            cout << "userid:";
            cin >> id;
            cin.get(); // 读掉缓冲区残留的回车
            cout << "userpassword:";
            cin.getline(pwd, 50);

            json js;
            js["msgId"] = LOGIN_MSG;
            js["id"] = id;
            js["password"] = pwd;
            string request = js.dump();

            g_isLoginSuccess = false;

            int len = send(clientfd, request.c_str(), strlen(request.c_str()) + 1, 0);
            if (len == -1)
            {
                cerr << "send login msg error:" << request << endl;
            }

            sem_wait(&rwsem); // 等待信号量，由子线程处理完登录的响应消息后，通知这里
                
            if (g_isLoginSuccess) 
            {
                // 进入聊天主菜单页面
                isMainMenuRunning = true;
                mainMenu(clientfd,id);
            }
        }
        break;
        case 2: // register业务
        {
            char name[50] = {0};
            char pwd[50] = {0};
            cout << "username:";
            cin.getline(name, 50);
            cout << "userpassword:";
            cin.getline(pwd, 50);

            json js;
            js["msgId"] = REG_MSG;
            js["name"] = name;
            js["password"] = pwd;
            string request = js.dump();

            int len = send(clientfd, request.c_str(), strlen(request.c_str()) + 1, 0);
            if (len == -1)
            {
                cerr << "send reg msg error:" << request << endl;
            }
            
            sem_wait(&rwsem); // 等待信号量，子线程处理完注册消息会通知
        }
        break;
        case 3: // quit业务
            close(clientfd);
            sem_destroy(&rwsem);
            exit(0);
        default:
            cerr << "invalid input!" << endl;
            break;
        }
    }

    return 0;
}

// 处理注册的响应逻辑
void doRegResponse(json &responsejs)
{
    if (0 != responsejs["errno"].get<int>()) // 注册失败
    {
        cerr << "name is already exist, register error!" << endl;
    }
    else // 注册成功
    {
        cout << "name register success, userid is " << responsejs["id"]
                << ", do not forget it!" << endl;
    }
}

// 处理登录的响应逻辑
void doLoginResponse(json &responsejs)
{

    if (0 != responsejs["errno"].get<int>()) // 登录失败
    {
        cerr << responsejs["errMsg"] << endl;
        g_isLoginSuccess = false;
    }
    else // 登录成功
    {
        // 记录当前用户的id和name
        g_currentUser.setId(responsejs["id"].get<int>());
        g_currentUser.setName(responsejs["name"]);

        // 记录当前用户的好友列表信息
        if (responsejs.contains("friends"))
        {
            // 初始化
            g_currentUserFriendList.clear();

            vector<string> vec = responsejs["friends"];
            for (string &str : vec)
            {
                json js = json::parse(str);
                User user;
                user.setId(js["id"].get<int>());
                user.setName(js["name"]);
                user.setSTATE(js["state"]);
                g_currentUserFriendList.push_back(user);
            }
        }

        // 记录当前用户的群组列表信息
        if (responsejs.contains("groups"))
        {
            // 初始化
            g_currentUserGroupList.clear();

            vector<string> vec1 = responsejs["groups"];
            for (string &groupstr : vec1)
            {
                json grpjs = json::parse(groupstr);
                Group group;
                group.setId(grpjs["id"].get<int>());
                group.setName(grpjs["groupname"]);
                group.setDesc(grpjs["groupdesc"]);

                vector<string> vec2 = grpjs["users"];
                for (string &userstr : vec2)
                {
                    GroupUser user;
                    json js = json::parse(userstr);
                    user.setId(js["id"].get<int>());
                    user.setName(js["name"]);
                    user.setSTATE(js["state"]);
                    user.setRole(js["role"]);
                    group.getUsers().push_back(user);
                }

                g_currentUserGroupList.push_back(group);
            }
        }

        // 显示登录用户的基本信息
        showCurrentUserData();

        // 显示当前用户的离线消息  个人聊天信息或者群组消息
        if (responsejs.contains("offlinemsg"))
        {
            vector<string> vec = responsejs["offlinemsg"];
            for (string &str : vec)
            {
                json js = json::parse(str);
                // time + [id] + name + " said: " + xxx
                if (ONE_CHAT_MSG == js["msgid"].get<int>())
                {
                    cout << js["time"].get<string>() << " [" << js["id"] << "]" << js["name"].get<string>()
                            << " said: " << js["msg"].get<string>() << endl;
                }
                else
                {
                    cout << "群消息[" << js["groupid"] << "]:" << js["time"].get<string>() << " [" << js["id"] << "]" << js["name"].get<string>()
                            << " said: " << js["msg"].get<string>() << endl;
                }
            }
        }

        g_isLoginSuccess = true;
    }
}

// 子线程 - 接收线程
void readTaskHandler(int clientfd)
{
    vector<string> jsonVec;
    for (;;)
    {
        char buffer[1024] = {0};
        // 接收ChatServer转发的数据，反序列化生成json数据对象
        json js;
        if(jsonVec.empty())
        {
            int len = recv(clientfd, buffer, 1024, 0);  // 阻塞了
            if (-1 == len || 0 == len)
            {
                close(clientfd);
                exit(-1);
            }
            js = json::parse(buffer);

        }
        else
        {
            js = json::parse(jsonVec.back());
            jsonVec.pop_back();
        }

        int msgtype = js["msgId"].get<int>();

        if(msgtype == JSON_VEC_MSG)
        {
            vector<string> receiveJsonVec = js["jsonVec"];
            for(auto str : receiveJsonVec)
            {
                jsonVec.push_back(str);
            }
            continue;
        }
        

        if (LOGIN_MSG_ACK == msgtype)
        {
            doLoginResponse(js); // 处理登录响应的业务逻辑
            sem_post(&rwsem);    // 通知主线程，登录结果处理完成
            continue;
        }

        if (REG_MSG_ACK == msgtype)
        {
            doRegResponse(js);
            sem_post(&rwsem);    // 通知主线程，注册结果处理完成
            continue;
        }

        if (LOGINOUT_MSG == msgtype)
        {
            if(js["errno"]!=0)
            {
                cerr<<"Loginout error!"<<endl;
                continue;
            }
                sem_post(&rwsem);
            
        }
        
        if(ONE_CHAT_MSG == msgtype)
        {

            if(js["errno"] == 1)
            {
                string err = js["errInfo"];
                cerr<<err<<endl;
            }
            else
            {
                cout<<"发送成功！";
                if(js["state"]=="online")cout<<"对方此时在线！"<<endl;
                else cout<<"对方此时离线"<<endl;
            }
            sem_post(&rwsem);
        }
        
        if(RECEIVE_MSG == msgtype)
        {
            if(js["errno"] == 0)
            {
                string msg = js["message"];
                string time = js["time"];
                string fromName = js["from"];
                cout<<time<<"\n收到:"<<msg<<" 来自:"<<fromName<<endl;
            }

        }
        if(ADD_FRIEND_MSG == msgtype)
        {
            if(js["errno"] == 0)
            {
                cout<<"添加成功!"<<endl;
            }
            else if(js["errno"] == 1)
            {
                cerr<<"对方已经是您的好友！"<<endl;
            }
            sem_post(&rwsem);
            continue;
        }

        if(msgtype == CREATE_GROUP_MSG)
        {
            if(js["errno"] == 0)
            {
                cout<<"创建群组成功！";
                int groupId = js["groupId"];
                cout<<"群号为："<<groupId<<endl;
            }
            else if(js["errno"] == 1)
            {
                cerr<<"群组已存在!"<<endl;
            }
            sem_post(&rwsem);
            continue;
        }

        if(msgtype == ADD_GROUP_MSG)
        {
            switch (js["errno"].get<int>())
            {
            case 0:
                cout<<"加入群组成功"<<endl;
                break;
            case 1:
                cerr<<"您已加入该群组!"<<endl;
                break;
            case 2:
                cerr<<"该群组不存在！"<<endl;
                break;
            }
            sem_post(&rwsem);
            continue;
        }
        
        if(msgtype == GROUP_CHAT_MSG)
        {
            switch ((int)js["errno"])
            {
            case 0:
                cout<<"发送成功"<<endl;
                break;
            case 1:
                cerr<<"您不在该群组!"<<endl;
                break;
            }
            sem_post(&rwsem);
            continue;
        }
        if(msgtype == RECEIVE_GROUP_MSG)
        {
            cout<<js["time"]<<endl;
            cout<<js["message"]<<"来自:"<<js["from"]<<"  来自群组:"<<js["groupId"]<<endl;
            continue;
        }
    }
}

// 显示当前登录成功用户的基本信息
void showCurrentUserData()
{
    cout << "======================login user======================" << endl;
    cout << "current login user => id:" << g_currentUser.getId() << " name:" << g_currentUser.getName() << endl;
    cout << "----------------------friend list---------------------" << endl;
    if (!g_currentUserFriendList.empty())
    {
        for (User &user : g_currentUserFriendList)
        {
            cout << user.getId() << " " << user.getName() << " " << user.getState() << endl;
        }
    }
    cout << "----------------------group list----------------------" << endl;
    if (!g_currentUserGroupList.empty())
    {
        for (Group &group : g_currentUserGroupList)
        {
            cout << group.getId() << " " << group.getName() << " " << group.getDesc() << endl;
            for (GroupUser &user : group.getUsers())
            {
                cout << user.getId() << " " << user.getName() << " " << user.getState()
                     << " " << user.getRole() << endl;
            }
        }
    }
    cout << "======================================================" << endl;
}

// "help" command handler
void help(int, string,int)
{
    cout << "show command list >>> " << endl;
    for (auto &p : commandMap)
    {
        cout << p.first << " : " << p.second << endl;
    }
    cout << endl;
}
// 主聊天页面程序
void mainMenu(int clientfd,int id)
{
    help();

    char buffer[1024] = {0};
    while (isMainMenuRunning)
    {
        cin.getline(buffer, 1024);
        string commandbuf(buffer);
        string command; // 存储命令
        int idx = commandbuf.find(":");
        if (-1 == idx)
        {
            command = commandbuf;
        }
        else
        {
            command = commandbuf.substr(0, idx);
        }
        auto it = commandHandlerMap.find(command);
        if (it == commandHandlerMap.end())
        {
            cerr << "invalid input command!" << endl;
            continue;
        }

        // 调用相应命令的事件处理回调，mainMenu对修改封闭，添加新功能不需要修改该函数
        it->second(clientfd, commandbuf.substr(idx + 1, commandbuf.size() - idx),id); // 调用命令处理方法
    }
}
// "chat" command handler
void chat(int clientFd, string info,int id)
{
    int idx = info.find(":");

    int friendId = stoi(info.substr(0,idx));
    if(friendId == id)
    {
        cerr<<"对不起，你不能给自己发送消息"<<endl;
        return;
    }
    string Msg = info.substr(idx+1,info.size()-1);
    json js;
    js["to"] = friendId;
    js["message"]=Msg;
    js["msgId"]=ONE_CHAT_MSG;
    js["id"]=id;
    string request = js.dump();
    int len = send(clientFd, request.c_str(), strlen(request.c_str()) + 1, 0);
    if(len == -1)cerr<<"Loginout Error!"<<endl;
    sem_wait(&rwsem);
}
// "addfriend" command handler
void addfriend(int clientFd , string info,int myId)
{
    json js;
    js["id"] = myId;
    int friendId = stoi(info);
    js["friendId"] = friendId;
    js["msgId"]= ADD_FRIEND_MSG;
    string request = js.dump();
    int len = send(clientFd, request.c_str(), strlen(request.c_str()) + 1, 0);
    if(len == -1)cerr<<"Loginout Error!"<<endl;
    sem_wait(&rwsem);
}
// "creategroup" command handler
void creategroup(int clientFd, string info,int id)
{
    int idx = info.find(":");
    string groupName = info.substr(0,idx);
    string groupDesc = info.substr(idx +1 ,info.size() -1);
    json js;
    js["groupName"] = groupName;
    js["groupDesc"] = groupDesc;
    js["id"]=id;
    js["msgId"]= CREATE_GROUP_MSG;
    string request = js.dump();
    int len = send(clientFd, request.c_str(), strlen(request.c_str()) + 1, 0);
    if(len == -1)cerr<<"Addgroup Error!"<<endl;
    sem_wait(&rwsem);
}
// "addgroup" command handler
void addgroup(int clientFd, string info,int id)
{
    json js;
    js["id"] =id;
    js["groupId"] = stoi(info);
    js["msgId"]=ADD_GROUP_MSG;
    string request = js.dump();
    int len = send(clientFd, request.c_str(), strlen(request.c_str()) + 1, 0);
    if(len == -1)cerr<<"Addgroup Error!"<<endl;
    sem_wait(&rwsem);
}
// "groupchat" command handler
void groupchat(int clientFd, string info,int id )
{
    json js;
    js["msgId"]=GROUP_CHAT_MSG;
    js["id"] = id;
    int idx = info.find(":");
    int groupId = stoi(info.substr(0,idx));
    string Msg = info.substr(idx+1,info.size()-1);
    js["groupId"] = groupId;
    js["message"] = Msg;
    string request = js.dump();
    int len = send(clientFd, request.c_str(), strlen(request.c_str()) + 1, 0);
    if(len == -1)cerr<<"Loginout Error!"<<endl;
    sem_wait(&rwsem);
}
// "loginout" command handler
void loginout(int clientFd, string,int id)
{
    json js;
    js["msgId"] = LOGINOUT_MSG;
    js["id"]= id;
    string request = js.dump();
    int len = send(clientFd, request.c_str(), strlen(request.c_str()) + 1, 0);
    if(len == -1)cerr<<"Loginout Error!"<<endl;
    sem_wait(&rwsem);
    exit(0);
}
