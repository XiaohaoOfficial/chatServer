#include "friendmodel.h"
#include "db.h"

//添加好友关系
void FriendModel::insert(int userId,int friendId)
{
    char sql[1024] = {0};
    sprintf(sql,"insert into Friend(userid,friendid) values(%d,%d)",userId,friendId);
    DataBase database;
    if(database.connect())
    {
        database.update(sql);
    }
}


//返回用户好友列表
vector<User> FriendModel::query(int userId)
{
    char sql[1024]={0};
    sprintf(sql,"SELECT a.id,a.name,a.state from User a INNER join Friend b on b.friendid = a.id where b.userid =  %d ",userId );
    DataBase database;
    vector<User> vec;
    if(database.connect())
    {
        auto res = database.query(sql);
        MYSQL_ROW row;
        while((row = mysql_fetch_row(res)) != nullptr)
        {
            string id = row[0];
            string name = row[1];
            string state = row[2];
            User user;
            user.setId(stoi(id));
            user.setName(name);
            user.setSTATE(state);
            vec.push_back(user);
        }
    }
    return vec;
}