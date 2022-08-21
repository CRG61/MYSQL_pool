#ifndef CONNECTIONPOOL_H
#define CONNECTIONPOOL_H

#include <string>
#include <queue>
#include <mutex>
#include <iostream>
#include <atomic>
#include <thread>
#include <condition_variable>
#include <memory>
#include <functional>
#include "Connection.h"

//连接池对象，单例模式——懒汉
class ConnectionPool
{
public:
    //获取对象实例
    static ConnectionPool *getConnectionPool();
    //从连接池获取空闲连接的接口
    std::shared_ptr<Connection> getConnection();

private:
    //单例，私有构造函数
    ConnectionPool();
    //读配置文件
    bool loadConfigFile();
    //运行在独立的线程中，专门负责生产新链接
    void produceConnection();
    //扫描闲置超过maxIdleTime的空闲链接并回收
    void scanConnection();

    std::string _ip;        // mysql server的ip地址
    unsigned short _port;   // mysql端口号 3306
    std::string _username;  // mysql登录用户名
    std::string _password;  // mysql登录密码
    std::string _dbname;    //连接的数据库名称
    int _initSize;          //链接池的初始链接数
    int _maxSize;           //链接池最大链接数
    int _maxIdleTime;       //最大空闲时间
    int _connectionTimeout; //客户端获取链接超时时间

    std::queue<Connection *> _connectionQue; //存储mysql链接的队列
    std::mutex _queueMutex;                 //维护连接队列的线程安全互斥锁
    std::atomic_int _connectionCnt;         //链接池中当前链接总数量
    std::condition_variable cv;             //条件变量，用于生产线程和消费线程间通信
};

#endif
