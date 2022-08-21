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

//���ӳض��󣬵���ģʽ��������
class ConnectionPool
{
public:
    //��ȡ����ʵ��
    static ConnectionPool *getConnectionPool();
    //�����ӳػ�ȡ�������ӵĽӿ�
    std::shared_ptr<Connection> getConnection();

private:
    //������˽�й��캯��
    ConnectionPool();
    //�������ļ�
    bool loadConfigFile();
    //�����ڶ������߳��У�ר�Ÿ�������������
    void produceConnection();
    //ɨ�����ó���maxIdleTime�Ŀ������Ӳ�����
    void scanConnection();

    std::string _ip;        // mysql server��ip��ַ
    unsigned short _port;   // mysql�˿ں� 3306
    std::string _username;  // mysql��¼�û���
    std::string _password;  // mysql��¼����
    std::string _dbname;    //���ӵ����ݿ�����
    int _initSize;          //���ӳصĳ�ʼ������
    int _maxSize;           //���ӳ����������
    int _maxIdleTime;       //������ʱ��
    int _connectionTimeout; //�ͻ��˻�ȡ���ӳ�ʱʱ��

    std::queue<Connection *> _connectionQue; //�洢mysql���ӵĶ���
    std::mutex _queueMutex;                 //ά�����Ӷ��е��̰߳�ȫ������
    std::atomic_int _connectionCnt;         //���ӳ��е�ǰ����������
    std::condition_variable cv;             //�������������������̺߳������̼߳�ͨ��
};

#endif
