#include "ConnectionPool.h"
#include "public.h"

//线程安全的懒汉单例接口
//懒汉不使用双重校验锁时，严格意义上不算单例（多线程会产生多个实例）
ConnectionPool *ConnectionPool::getConnectionPool()
{
    static ConnectionPool pool; // lock 和 unlock (?)
    return &pool;
}

// 从配置文件加载配置参数
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
        std::string str = line; //可以使用c风格字符串给string赋值
        int idx = str.find('=', 0);
        if (idx == -1) // 无效的配置项
        {
            continue;
        }

        // password=123456\n
        int endidx = str.find('\n', idx);
        std::string key = str.substr(0, idx); //第二个参数是复制的长度
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

// 连接池构造函数
ConnectionPool::ConnectionPool()
{
    //加载配置项
    if (!loadConfigFile())
    {
        return;
    }

    //创建初始数量的连接
    for (int i = 0; i != _initSize; ++i)
    {
        Connection *p = new Connection();
        p->connect(_ip, _port, _username, _password, _dbname);
        p->refreshAliveTime(); //刷新空闲时间起始点
        _connectionQue.push(p);
        _connectionCnt++;
    }

    // std::thread是一个类，produce应该是类实例化的对象，查查Bind的用法。这个类大概率是重载了调用运算符
    //启动一个新的线程，作为生产者生产连接 linux thread->pthread_create
    std::thread produce(std::bind(&ConnectionPool::produceConnection, this));
    produce.detach();

    //启动一个新线程，扫描空闲时间超过maxIdleTime的闲置连接，并回收
    std::thread scanner(std::bind(&ConnectionPool::scanConnection, this));
    scanner.detach();
}

// 运行在独立的线程中，专门负责生产新连接
void ConnectionPool::produceConnection()
{
    for (;;)
    {
        // C++加锁的方法要去查一下
        std::unique_lock<std::mutex> lock(_queueMutex);
        while (!_connectionQue.empty())
        {
            // C++的条件变量语法也和Linux不同，查一下
            cv.wait(lock); //队列不空，则生产线程阻塞等待   被唤醒后又会while循环检查队列，只有队列空了才会真的创建新连接
        }

        //连接数量未达上限，创建连接
        if (_connectionCnt < _maxSize)
        {
            Connection *p = new Connection();
            p->connect(_ip, _port, _username, _password, _dbname);
            p->refreshAliveTime();
            _connectionQue.push(p);
            _connectionCnt++;
        }

        // 通知消费者线程消费连接
        cv.notify_all();
    }
}

//接口，从线程池获取可用连接
std::shared_ptr<Connection> ConnectionPool::getConnection()
{
    std::unique_lock<std::mutex> lock(_queueMutex);
    while (_connectionQue.empty())
    {
        if (std::cv_status::timeout == cv.wait_for(lock, std::chrono::milliseconds(_connectionTimeout)))
        {
            if (_connectionQue.empty())
            {
                LOG("获取空闲链接超时");
                return nullptr;
            }
        }
    }

    /*
        shared_ptr智能指针析构时，会把connection资源直接delete掉，相当于
        调用connection的析构函数，connection就被close掉了。
        这里需要自定义shared_ptr的释放资源的方式，把connection直接归还到queue当中
    */

    std::shared_ptr<Connection> sp(_connectionQue.front(),
                                   [&](Connection *pcon)
                                   {//这个捕获范围应该限定在shared_ptr本身
                                       // 这里是在服务器应用线程中调用的，所以一定要考虑队列的线程安全操作
                                       std::unique_lock<std::mutex> lock(_queueMutex);
                                       pcon->refreshAliveTime();
                                       _connectionQue.push(pcon);
                                   });
    _connectionQue.pop();
    cv.notify_all(); // 取走一个连接以后，通知生产者线程检查一下，如果队列为空了，赶紧生产连接
    return sp;
}

void ConnectionPool::scanConnection()
{
    for (;;)
    {
        //通过sleep模拟定时效果
        std::this_thread::sleep_for(std::chrono::seconds(_maxIdleTime));

        //只需要检查队头的连接是否超时，因为是按顺序放入的
        std::unique_lock<std::mutex> lock(_queueMutex);
        while (_connectionCnt > _initSize)
        {
            Connection *p = _connectionQue.front();
            if (p->getAliveTime() >= (_maxIdleTime * 1000))
            {
                _connectionQue.pop();
                _connectionCnt--;
                delete p; // 调用~Connection()释放连接
            }
            else
            {
                break; // 队头的连接没有超过_maxIdleTime，其它连接肯定没有
            }
        }
    }
}