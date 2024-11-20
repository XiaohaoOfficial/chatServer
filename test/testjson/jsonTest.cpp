#include "json.hpp"
using json = nlohmann::json;

#include <iostream>
#include <vector>
#include <map>
#include <string>

using namespace std;
string func1()
{
    json js;
    js["msg_type"] = 2;
    js["from"] = "zhang san";
    js["to"] = "li si";
    js["msg"] = "hello,where are you going now";

    cout<<js<<endl;
    
    string sendBuf = js.dump();
    cout<<sendBuf<<endl;
    return sendBuf;
}

void func2()
{
    json js;
    js["id"]={1,2,3,4,5};
    js["name"]="zhang san";
    js["msg"]["zhang san"]="hello world";
    js["msg"]["li si"]="hello china";
    js["msg"]={{"zhang san","hello world"},{"lisi","hello china"}};

    cout<<js<<endl;
}

void func3()
{
    json js;

    vector<int> vec;
    for (int i = 0; i < 5 ; ++i) 
    {
        vec.push_back(i);
    }
    js["vector"] = vec;


    map<int,string> mp;
    mp.insert({1,"hello world"});    
    mp.insert({2,"你好中国"});    
    mp.insert({3,"世界你好"});    

    js["map"] = mp;

    cout<<js<<endl;

}
int main()
{
    func1();
    cout<<"------------------------------------------------\n";
    func2();
    cout<<"------------------------------------------------\n";
    func3();
    cout<<"------------------------------------------------\n";

    string receBuf = func1();
    json js = json::parse(receBuf);
    cout<<"------------------------------------------------\n";

    cout<<js["msg"]<<endl;
    return 0;
}