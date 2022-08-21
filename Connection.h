#ifndef CONNECTION_H
#define CONNECTION_H

#include <mysql.h>
#include <string>
#include <ctime>

/*
MYSQL���ݿ������
*/
class Connection
{
    public:
    Connection();
    ~Connection();
    // �������ݿ�
    bool connect(std::string ip, 
    unsigned short port, 
    std::string user, 
    std::string password, 
    std::string dbname);

    //���²��� ����insert��delete��update
    bool update(std::string sql);
    //��ѯ����
    MYSQL_RES* query(std::string sql);

    //ˢ�����ӿ���ʱ����ʼ��
    void refreshAliveTime() {_alivetime = clock();}
    //���ش���ʱ��
    clock_t getAliveTime(){ return clock() - _alivetime;}

    private:
    MYSQL *_conn;  //��MYSQL server��һ������
    clock_t _alivetime;
};
#endif