#pragma once

#include<zookeeper/zookeeper.h>
#include<semaphore.h>
#include"logger.hpp"

// 封装的zk客户端类
class ZkClient
{
public:
    ZkClient();
    ~ZkClient();
    // zkclient启动链接zkserver
    void Start();
    // 在zkserver上根据指定的路径创建znode节点
    void Create(const char *path, const char *data, int datalen, int state = 0);
    // 根据参数指定的znode节点路径，获取znode节点的值
    std::string GetData(const char* path);
    // 返回zookeeper操作句柄
    zhandle_t* getZHandle();
private:
    // zk的客户端句柄
    zhandle_t *m_zhandle;
};