#include "public.h"
#include "Connection.h"
#include <iostream>

Connection::Connection()
{
    //初始化数据库连接   这是mysql接口函数 （据网上的说法，分配一个栈空间似乎更好）
    _conn = mysql_init(nullptr);
}

Connection::~Connection()
{
    //释放数据库连接
    if (_conn != nullptr)
    {
        mysql_close(_conn);
        _conn = nullptr;
    }
        
}

bool Connection::connect(std::string ip, unsigned short port, std::string username, std::string password, std::string dbname)
{
    //连接数据库   （对于成功的连接，返回值和第一个参数值相同）
    MYSQL *p = mysql_real_connect(_conn, ip.c_str(), username.c_str(), password.c_str(), dbname.c_str(), port, nullptr, 0);
    return p != nullptr;
}

bool Connection::update(std::string sql)
{
    //更新操作 insert/delete/update
    if(mysql_query(_conn, sql.c_str()))
    {
        LOG("更新失败："+ sql);
        return false;
    }
    return true;
}

MYSQL_RES* Connection::query(std::string sql)
{
    //查询操作 select
    if(mysql_query(_conn,sql.c_str()))
    {
        LOG("更新失败："+ sql);
        return nullptr;
    }
    return mysql_use_result(_conn);
}