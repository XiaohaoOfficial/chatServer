#ifndef PUBLIC_H
#define PUBLIC_H

/*
server和client的公共文件
*/

enum EnMsgType
{
    LOGIN_MSG = 1,
    LOGIN_MSG_ACK,
    REG_MSG,
    REG_MSG_ACK,
    ONE_CHAT_MSG,
    RECEIVE_MSG,
    ADD_FRIEND_MSG,
    CREATE_GROUP_MSG,
    ADD_GROUP_MSG,
    GROUP_CHAT_MSG,
    LOGINOUT_MSG,
    RECEIVE_GROUP_MSG,
    JSON_VEC_MSG
    
};
enum STATE
{
    ONLINE,
    OFFLINE
};

#endif