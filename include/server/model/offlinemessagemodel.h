#ifndef OFFLINEMESSAGEMODEL_H
#define OFFLINEMESSAGEMODEL_H
#include <string>
#include <vector>
using namespace std;
class OfflineMsgModel
{
public:
    //存储用户的离线信息
    bool insert(int userId,string msg);
    //删除用户的离线信息
    bool remove(int userId);
    //查询用户的离线信息
    vector<string> query(int userId);

private:
};

#endif