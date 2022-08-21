#ifndef CONNECTION_H
#define CONNECTION_H

#include <mysql.h>
#include <string>
#include <ctime>

/*
MYSQL数据库操作类
*/
class Connection
{
    public:
    Connection();
    ~Connection();
    // 连接数据库
    bool connect(std::string ip, 
    unsigned short port, 
    std::string user, 
    std::string password, 
    std::string dbname);

    //更新操作 ——insert、delete、update
    bool update(std::string sql);
    //查询操作
    MYSQL_RES* query(std::string sql);

    //刷新链接空闲时间起始点
    void refreshAliveTime() {_alivetime = clock();}
    //返回存活的时间
    clock_t getAliveTime(){ return clock() - _alivetime;}

    private:
    MYSQL *_conn;  //和MYSQL server的一条链接
    clock_t _alivetime;
};
#endif