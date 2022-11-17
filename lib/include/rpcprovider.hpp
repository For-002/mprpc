#pragma once

#include"google/protobuf/service.h"
#include<muduo/net/TcpServer.h>
#include<muduo/net/EventLoop.h>
#include<muduo/net/InetAddress.h>
#include<muduo/net/TcpConnection.h>
#include<unordered_map>
#include"google/protobuf/descriptor.h"

class RpcProvider
{
public:
    void NotifyService(google::protobuf::Service *);

    void Run();
private:
    // 组合 EventLoop
    muduo::net::EventLoop m_eventloop;

    // 保存服务信息
    struct ServiceInfo
    {
        google::protobuf::Service *m_service;   // 保存服务对象
        std::unordered_map<std::string, const google::protobuf::MethodDescriptor*> m_methodMap;   // 保存服务的方法信息
    };

    // 存储在 rpc 框架上注册的所有服务
    std::unordered_map<std::string, ServiceInfo> m_serviceMap;
    
    // 新的 socket 连接回调
    void OnConnection(const muduo::net::TcpConnectionPtr&);
    // 接收到新消息地回调 
    void OnMessage(const muduo::net::TcpConnectionPtr&,
                            muduo::net::Buffer*,
                            muduo::Timestamp);
    // 
    void SendRpcResponse(const muduo::net::TcpConnectionPtr&, google::protobuf::Message*);
};
