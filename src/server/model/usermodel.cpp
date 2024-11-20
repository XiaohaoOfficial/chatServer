#include "usermodel.h"
#include "db.h"

bool UserModel::insert(User &user)
{
    char sql[1024] = {0};
    sprintf(sql,"insert into User(name,password,state) values('%s','%s','%s')",
    user.getName().c_str(),user.getPassword().c_str(),user.getState().c_str());

    DataBase database;
    if(database.connect())
    {
        if(database.update(sql))
        {
            user.setId(mysql_insert_id(database.getConnection()));
            return true;
        }
    }

    return false;
}
User UserModel::query(int id)
{
    char sql[1024]={0};
    sprintf(sql,"select * from User where id = %d",id);

    DataBase database;
    if(database.connect())
    {
        auto res = database.query(sql);
        if(res != nullptr)
        {
            auto rows = mysql_fetch_row(res);
            User user;
            user.setId(atoi(rows[0]));
            user.setName(rows[1]);
            user.setPassword(rows[2]);
            user.setSTATE(rows[3]);
            return user;
        }
    }
    return User();
}
bool UserModel::updateState(User user,STATE state)
{
    char sql[1024] = {0};
    string userState = "offline";
    if(state == ONLINE)userState ="online";

    sprintf(sql,"update User set state = '%s' where id = %d",userState.c_str(),user.getId());
    DataBase database;
    if(database.connect())
    {
        return database.update(sql);
    }
    return false;
}