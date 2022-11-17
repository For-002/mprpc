#include"zookeeperutil.hpp"
#include"mprpcapplication.hpp"

ZkClient::ZkClient() : m_zhandle(nullptr)
{
}

ZkClient::~ZkClient()
{
    if (m_zhandle != nullptr)
        zookeeper_close(m_zhandle); // 关闭句柄，释放资源
}

// 全局watcher观察器，若连接成功zkserver会发送响应消息
void global_watcher(zhandle_t *zh, int type, int state, const char *path, void *watcherCtx)
{
    if (type == ZOO_SESSION_EVENT)  // ZOO_SESSION_EVENT表示消息类型是和建立会话相关的消息类型
    {
        if (state == ZOO_CONNECTED_STATE)   // ZOO_CONNECTED_STATE表示zkclient和zkserver连接成功
        {
            sem_t *sem = (sem_t*)zoo_get_context(zh);   // 从指定句柄上获取信号量
            sem_post(sem);  // 信号量+1，通知API调用线程 和zkserver连接成功
        }
    }
}

void ZkClient::Start()
{
    std::string ip = MprpcApplication::GetInstance().GetConfig().LoadConfig("zookeeperip");
    std::string port = MprpcApplication::GetInstance().GetConfig().LoadConfig("zookeeperport");
    std::string connstr = ip + ":" + port;

    /*
    zookeeper_mt: 这是zookeeper的多线程版本
    调用zookeeper_init会额外开启两个线程
        网 络 I/O 线 程：专门负责与zookeeper服务器进行网络连接
        watcher回调线程：监听zookeeper上的各个节点，当服务器对连接请求发出回应，global_watcher线程就会收到这个回应并进行判断连接服务器是否成功
    该函数的返回值仅表示创建m_zhandle句柄是否成功，并不代表连接是否成功。
    */
    m_zhandle = zookeeper_init(connstr.c_str(), global_watcher, 30000, nullptr, nullptr, 0);
    if (nullptr == m_zhandle)
    {
        //LOG_ERR("Zookeeper_init error!  %s-%s-%d", __FILE__, __FUNCTION__, __LINE__);
        exit(EXIT_FAILURE);
    }

    sem_t sem;
    sem_init(&sem, 0, 0);
    // 可以理解为给指定句柄添加额外的信息，这里添加了一个信号量，方便API调用线程和global_watcher回调线程进行同步
    zoo_set_context(m_zhandle, &sem);

    // (API调用线程)阻塞在这里，等待global_watcher回调线程的消息
    sem_wait(&sem);
    //LOG_INFO("zookeeper_init success!");
    // 调试语句
    std::cout << "zookeeper_init success!" << std::endl;
}

zhandle_t* ZkClient::getZHandle()
{
    return this->m_zhandle;
}

/**
 * path：结点的路径
 * data：节点存储的数据
 * datalen：数据的长度
 * state：节点是永久节点还是临时节点
 */
void ZkClient::Create(const char *path, const char *data, int datalen, int state)
{
    char path_buffer[128] = {0};
    int bufferlen = sizeof(path_buffer);
    int flag;
    // 先确保目标路径下还没有创建该节点
    flag = zoo_exists(m_zhandle, path, 0, nullptr);
    if (flag == ZNONODE)
    {
        // 创建节点
        flag = zoo_create(m_zhandle, path, data, datalen, &ZOO_OPEN_ACL_UNSAFE, state, path_buffer, bufferlen);
        if (flag == ZOK)
        {
            //LOG_INFO("%s created successfully!", path);
        }
        else
        {
            //LOG_ERR("Znode create error! Path:%s  flag:%d  %s--%s--%d", path, flag, __FILE__, __FUNCTION__, __LINE__);
            exit(EXIT_FAILURE);
        }
    }
    else
    {
        //LOG_ERR("The node is existed! Path:%s", path);
    }
}

std::string ZkClient::GetData(const char *path)
{
    char buffer[128] = {0};
    int bufferlen = sizeof(buffer);
    int flag = zoo_get(m_zhandle, path, 0, buffer, &bufferlen, nullptr);
    if (ZOK != flag)
    {
        //LOG_ERR("Get znode error! Path:%s  %s--%s--%d", path, __FILE__, __FUNCTION__, __LINE__);
        return "";
    }
    else
    {
        return buffer;
    }
}