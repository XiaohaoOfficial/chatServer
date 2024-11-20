#ifndef USER_H
#define USER_H

#include <string>
#include "public.h"
using namespace std;
class User
{
public:

    User(int id = -1,string name = "",string password = "",STATE state = OFFLINE):
    _id(id),_name(name),_password(password),_state(state){}
    int getId(){return _id;}
    string getName(){return _name;}
    string getPassword(){return _password;}
    string getState()
    {
        if(_state == ONLINE)return "ONLINE";
        else return "OFFLINE";
    }
    void setId(int id){_id = id;}
    void setName(string name){_name = name;}
    void setPassword(string password){_password = password;}
    void setSTATE(string state)
    {
        if(state == "online")_state = ONLINE;
        else _state = OFFLINE;
    }
private:
    int _id;
    string _name;
    string _password;
    STATE _state;
    
};






#endif