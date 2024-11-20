#include "chatserver.h"
#include "chatservice.h"
#include <iostream>
#include <signal.h>
using namespace std;

//ctrl+C 结束时，将所有user状态改为offline
void resetHandle(int)
{
    ChatService::getInstance()->resetState();
    ChatService::getInstance()->unSubscribeRedis();
    exit(0);
}
int main(int argc,char **argv)
{


    if (argc < 3)
    {
        cerr << "command invalid! example: ./ChatServer 127.0.0.1 6000" << endl;
        exit(-1);
    }
    char *ip = argv[1];
    char *port = argv[2];

    signal(SIGINT,resetHandle);
    EventLoop loop;
    InetAddress addr(ip,stoi(port));
    ChatServer server(&loop,addr,"chatserver");

    server.start();
    loop.loop();
}