#include "redis.h"
#include <iostream>
#include <map>
using namespace std;

int main() 
{
    Redis redis;
    redis.connect();
    return 0;
}