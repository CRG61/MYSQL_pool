#include "public.h"
#include "Connection.h"
#include <iostream>

Connection::Connection()
{
    //��ʼ�����ݿ�����   ����mysql�ӿں��� �������ϵ�˵��������һ��ջ�ռ��ƺ����ã�
    _conn = mysql_init(nullptr);
}

Connection::~Connection()
{
    //�ͷ����ݿ�����
    if (_conn != nullptr)
    {
        mysql_close(_conn);
        _conn = nullptr;
    }
        
}

bool Connection::connect(std::string ip, unsigned short port, std::string username, std::string password, std::string dbname)
{
    //�������ݿ�   �����ڳɹ������ӣ�����ֵ�͵�һ������ֵ��ͬ��
    MYSQL *p = mysql_real_connect(_conn, ip.c_str(), username.c_str(), password.c_str(), dbname.c_str(), port, nullptr, 0);
    return p != nullptr;
}

bool Connection::update(std::string sql)
{
    //���²��� insert/delete/update
    if(mysql_query(_conn, sql.c_str()))
    {
        LOG("����ʧ�ܣ�"+ sql);
        return false;
    }
    return true;
}

MYSQL_RES* Connection::query(std::string sql)
{
    //��ѯ���� select
    if(mysql_query(_conn,sql.c_str()))
    {
        LOG("����ʧ�ܣ�"+ sql);
        return nullptr;
    }
    return mysql_use_result(_conn);
}