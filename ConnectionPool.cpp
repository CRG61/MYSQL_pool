#include "ConnectionPool.h"
#include "public.h"

//�̰߳�ȫ�����������ӿ�
//������ʹ��˫��У����ʱ���ϸ������ϲ��㵥�������̻߳�������ʵ����
ConnectionPool *ConnectionPool::getConnectionPool()
{
    static ConnectionPool pool; // lock �� unlock (?)
    return &pool;
}

// �������ļ��������ò���
bool ConnectionPool::loadConfigFile()
{
    FILE *pf = fopen("mysql.ini", "r");
    if (pf == nullptr)
    {
        LOG("mysql.ini file is not exist!");
        return false;
    }

    while (!feof(pf))
    {
        char line[1024] = {0};
        fgets(line, 1024, pf);
        std::string str = line; //����ʹ��c����ַ�����string��ֵ
        int idx = str.find('=', 0);
        if (idx == -1) // ��Ч��������
        {
            continue;
        }

        // password=123456\n
        int endidx = str.find('\n', idx);
        std::string key = str.substr(0, idx); //�ڶ��������Ǹ��Ƶĳ���
        std::string value = str.substr(idx + 1, endidx - idx - 1);

        if (key == "ip")
        {
            _ip = value;
        }
        else if (key == "port")
        {
            _port = atoi(value.c_str());
        }
        else if (key == "username")
        {
            _username = value;
        }
        else if (key == "password")
        {
            _password = value;
        }
        else if (key == "dbname")
        {
            _dbname = value;
        }
        else if (key == "initSize")
        {
            _initSize = atoi(value.c_str());
        }
        else if (key == "maxSize")
        {
            _maxSize = atoi(value.c_str());
        }
        else if (key == "maxIdleTime")
        {
            _maxIdleTime = atoi(value.c_str());
        }
        else if (key == "connectionTimeOut")
        {
            _connectionTimeout = atoi(value.c_str());
        }
    }
    return true;
}

// ���ӳع��캯��
ConnectionPool::ConnectionPool()
{
    //����������
    if (!loadConfigFile())
    {
        return;
    }

    //������ʼ����������
    for (int i = 0; i != _initSize; ++i)
    {
        Connection *p = new Connection();
        p->connect(_ip, _port, _username, _password, _dbname);
        p->refreshAliveTime(); //ˢ�¿���ʱ����ʼ��
        _connectionQue.push(p);
        _connectionCnt++;
    }

    // std::thread��һ���࣬produceӦ������ʵ�����Ķ��󣬲��Bind���÷�������������������˵��������
    //����һ���µ��̣߳���Ϊ�������������� linux thread->pthread_create
    std::thread produce(std::bind(&ConnectionPool::produceConnection, this));
    produce.detach();

    //����һ�����̣߳�ɨ�����ʱ�䳬��maxIdleTime���������ӣ�������
    std::thread scanner(std::bind(&ConnectionPool::scanConnection, this));
    scanner.detach();
}

// �����ڶ������߳��У�ר�Ÿ�������������
void ConnectionPool::produceConnection()
{
    for (;;)
    {
        // C++�����ķ���Ҫȥ��һ��
        std::unique_lock<std::mutex> lock(_queueMutex);
        while (!_connectionQue.empty())
        {
            // C++�����������﷨Ҳ��Linux��ͬ����һ��
            cv.wait(lock); //���в��գ��������߳������ȴ�   �����Ѻ��ֻ�whileѭ�������У�ֻ�ж��п��˲Ż���Ĵ���������
        }

        //��������δ�����ޣ���������
        if (_connectionCnt < _maxSize)
        {
            Connection *p = new Connection();
            p->connect(_ip, _port, _username, _password, _dbname);
            p->refreshAliveTime();
            _connectionQue.push(p);
            _connectionCnt++;
        }

        // ֪ͨ�������߳���������
        cv.notify_all();
    }
}

//�ӿڣ����̳߳ػ�ȡ��������
std::shared_ptr<Connection> ConnectionPool::getConnection()
{
    std::unique_lock<std::mutex> lock(_queueMutex);
    while (_connectionQue.empty())
    {
        if (std::cv_status::timeout == cv.wait_for(lock, std::chrono::milliseconds(_connectionTimeout)))
        {
            if (_connectionQue.empty())
            {
                LOG("��ȡ�������ӳ�ʱ");
                return nullptr;
            }
        }
    }

    /*
        shared_ptr����ָ������ʱ�����connection��Դֱ��delete�����൱��
        ����connection������������connection�ͱ�close���ˡ�
        ������Ҫ�Զ���shared_ptr���ͷ���Դ�ķ�ʽ����connectionֱ�ӹ黹��queue����
    */

    std::shared_ptr<Connection> sp(_connectionQue.front(),
                                   [&](Connection *pcon)
                                   {//�������ΧӦ���޶���shared_ptr����
                                       // �������ڷ�����Ӧ���߳��е��õģ�����һ��Ҫ���Ƕ��е��̰߳�ȫ����
                                       std::unique_lock<std::mutex> lock(_queueMutex);
                                       pcon->refreshAliveTime();
                                       _connectionQue.push(pcon);
                                   });
    _connectionQue.pop();
    cv.notify_all(); // ȡ��һ�������Ժ�֪ͨ�������̼߳��һ�£��������Ϊ���ˣ��Ͻ���������
    return sp;
}

void ConnectionPool::scanConnection()
{
    for (;;)
    {
        //ͨ��sleepģ�ⶨʱЧ��
        std::this_thread::sleep_for(std::chrono::seconds(_maxIdleTime));

        //ֻ��Ҫ����ͷ�������Ƿ�ʱ����Ϊ�ǰ�˳������
        std::unique_lock<std::mutex> lock(_queueMutex);
        while (_connectionCnt > _initSize)
        {
            Connection *p = _connectionQue.front();
            if (p->getAliveTime() >= (_maxIdleTime * 1000))
            {
                _connectionQue.pop();
                _connectionCnt--;
                delete p; // ����~Connection()�ͷ�����
            }
            else
            {
                break; // ��ͷ������û�г���_maxIdleTime���������ӿ϶�û��
            }
        }
    }
}