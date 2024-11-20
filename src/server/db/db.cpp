#include "db.h"

    DataBase::DataBase()
    {
        _conn = mysql_init(nullptr);
    }

    DataBase::~DataBase()
    {
        if (_conn != nullptr)
            mysql_close(_conn);
    }
    // 连接数据库
    bool DataBase::connect()
    {
        MYSQL *p = mysql_real_connect(_conn, server.c_str(), user.c_str(),
                                      password.c_str(), dbname.c_str(), 3306, nullptr, 0);
        if (p != nullptr)
        {
            mysql_query(_conn,
                        "set names 'utf8mb4'");
            LOG_INFO<<"MYSQL CONNECT";
        }
        return p;
    }
    // 更新操作
    bool DataBase::update(string sql)
    {
        if (mysql_query(_conn, sql.c_str()))
        {
            LOG_INFO << __FILE__ << ":" << __LINE__ << ":"
                     << sql << "更新失败!";
            return false;
        }
        return true;
    }
    // 查询操作
    MYSQL_RES* DataBase::query(string sql)
    {
        if (mysql_query(_conn, sql.c_str()))
        {
            LOG_INFO << __FILE__ << ":" << __LINE__ << ":"
                     << sql << "查询失败!";
            return nullptr;
        }
        return mysql_use_result(_conn);
    }
    MYSQL* DataBase::getConnection()
    {
        return _conn;
    }
