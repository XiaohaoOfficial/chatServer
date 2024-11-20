#include "offlinemessagemodel.h"
#include "db.h"

bool OfflineMsgModel::insert(int userId,string msg)
{
    char sql[1024] = {0};
    sprintf(sql,"insert into OfflineMessage(userid,message) values(%d,'%s')",userId,msg.c_str());
    DataBase database;
    if(database.connect())
    {
        if(database.update(sql))
        {
            return true;
        }
    }
    return false;

}
    //删除用户的离线信息
bool OfflineMsgModel::remove(int userId)
{
    char sql[1024] = {0};
    sprintf(sql,"delete from OfflineMessage where userid = %d",userId);
    DataBase database;
    if(database.connect())
    {
        if(database.update(sql))
        return true;
    }
    return false;
}
    //查询用户的离线信息
vector<string> OfflineMsgModel::query(int userId)
{
    char sql[1024] = {0};
    sprintf(sql,"select * from OfflineMessage where userid = %d",userId);
    DataBase database;
    vector<string> vec;
    if(database.connect())
    {
        auto it = database.query(sql);
        MYSQL_ROW row;
        while((row = mysql_fetch_row(it) )!= nullptr)
        {
            vec.push_back(row[1]);
        }
        mysql_free_result(it);
    }
    return vec;
}