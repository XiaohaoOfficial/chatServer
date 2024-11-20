#ifndef FRIENDMODEL_H
#define FRIENDMODEL_H

#include <vector>
#include "user.h"
using namespace std;
class FriendModel
{
public:
    //添加好友关系
    void insert(int userId,int friendId);


    //返回用户好友列表
    vector<User> query(int userId);
private:


};




#endif