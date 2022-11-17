#include"rpcprovider.hpp"
#include<string>
#include"mprpcapplication.hpp"
#include<functional>
#include"rpcheader.pb.h"
#include"logger.hpp"
#include"zookeeperutil.hpp"

void RpcProvider::NotifyService(google::protobuf::Service *service)
{
    ServiceInfo service_info;
    service_info.m_service = service;

    // 获取服务的描述信息
    const google::protobuf::ServiceDescriptor *pserviceDesc = service->GetDescriptor();

    // 获取服务的名字
    std::string service_name = pserviceDesc->name();

    // 获取服务的方法数量
    int method_count = pserviceDesc->method_count();

    for (int i = 0; i < method_count; i++)
    {
        const google::protobuf::MethodDescriptor *pmethodDesc = pserviceDesc->method(i);
        std::string method_name = pmethodDesc->name();
        service_info.m_methodMap.insert({method_name, pmethodDesc});
    }

    m_serviceMap.insert({service_name, service_info});

    // 尝试打印
    std::cout << m_serviceMap.begin()->first << std::endl;
}

void RpcProvider::Run()
{
    std::string ip = MprpcApplication::GetConfig().LoadConfig("rpcserverip");
    uint16_t port = atoi(MprpcApplication::GetConfig().LoadConfig("rpcserverport").c_str());
    muduo::net::InetAddress address(ip, port);
    
    // 创建 TcpServer 对象
    muduo::net::TcpServer server(&this->m_eventloop, address, "RpcServer");

    // 绑定连接回调 和 消息读写回调方法
    server.setConnectionCallback(std::bind(&RpcProvider::OnConnection, this, std::placeholders::_1));
    server.setMessageCallback(std::bind(&RpcProvider::OnMessage, this, std::placeholders::_1, std::placeholders::_2, std::placeholders::_3));

    // 设置 muduo 库的线程数量
    server.setThreadNum(4);

    LOG_INFO("RpcProvider start service at ip: %s port:%d!", ip.c_str(), port);

    // 把当前rpc节点上要发布的服务全部注册到zk上面，让rpc client可以从zk上发现服务
    ZkClient zkcli;
    zkcli.Start();
    for (auto &sp : m_serviceMap)
    {
        std::string service_path = "/" + sp.first;
        int flag = zoo_exists(zkcli.getZHandle(), service_path.c_str(), 0, nullptr);
        // 服务节点设置为永久性节点
        if (flag == ZNONODE)
        {
            zkcli.Create(service_path.c_str(), nullptr, 0);
        }
        for (auto &mp : sp.second.m_methodMap)
        {
            std::string method_path = service_path + "/" + mp.first;
            char method_path_data[128] = {0};
            sprintf(method_path_data, "%s:%d", ip.c_str(), port);
            // 方法节点都是临时性节点
            zkcli.Create(method_path.c_str(), method_path_data, strlen(method_path_data), ZOO_EPHEMERAL);
        }
    }

    // 启动服务
    server.start();
    m_eventloop.loop();
}

void RpcProvider::OnConnection(const muduo::net::TcpConnectionPtr& conn)
{
    // 连接断开，回收文件描述符资源
    if (!conn->connected())
        conn->shutdown();
}

// 消息格式：header_size + service_name + method_name + args_size + args
void RpcProvider::OnMessage(const muduo::net::TcpConnectionPtr& conn,
                                    muduo::net::Buffer* buffer,
                                    muduo::Timestamp time)
{
    // 将tcp数据转换成字符流
    std::string recv_buf = buffer->retrieveAllAsString();

    // 获取 header 的长度
    uint32_t header_size = 0;
    recv_buf.copy((char*)&header_size, 4, 0);

    // 根据header的长度读取字符流，并将其反序列化，得到rpc请求的详细信息
    std::string rpc_header_str = recv_buf.substr(4, header_size);
    mprpc::RpcHeader rpcHeader;
    if (!rpcHeader.ParseFromString(rpc_header_str))
    {
        LOG_ERR("rpc_header_str: %s parse error!  %s--%s--%d", rpc_header_str.c_str(), __FILE__, __FUNCTION__, __LINE__);
        return;
    }

    std::string service_name = rpcHeader.service_name();
    std::string method_name = rpcHeader.method_name();
    uint32_t args_size = rpcHeader.args_size();

    // 得到rpc请求的参数的字符流
    std::string args_str = recv_buf.substr(4 + header_size, args_size);

    auto service_iter = m_serviceMap.find(service_name);
    if (m_serviceMap.end() == service_iter)
    {
        LOG_ERR("method: \"%s\" do not exist!  %s--%s--%d", service_name.c_str(), __FILE__, __FUNCTION__, __LINE__);
        return;
    }

    auto method_iter = service_iter->second.m_methodMap.find(method_name);
    if (service_iter->second.m_methodMap.end() == method_iter)
    {
        LOG_ERR("%s don't have: \"%s\" method!  %s--%s--%d", service_name.c_str(), method_name.c_str(), __FILE__, __FUNCTION__, __LINE__);
        return;
    }
    // 获取service对象
    google::protobuf::Service *service = service_iter->second.m_service;
    // 获取method对象
    const google::protobuf::MethodDescriptor *method = method_iter->second;
    
    // 生成 rpc 方法调用的 request 和 response 参数
    google::protobuf::Message *request = service->GetRequestPrototype(method).New();
    if (!request->ParseFromString(args_str))
    {
        LOG_ERR("request parse error, content: %s  %s--%s--%d", args_str.c_str(), __FILE__, __FUNCTION__, __LINE__);
        return ;
    }
    google::protobuf::Message *response = service->GetResponsePrototype(method).New();

    // 设置完成请求后的回调函数。创建一个Closure对象，使用想要设置的回调函数及其参数作为构造函数的参数。
    google::protobuf::Closure *done = google::protobuf::NewCallback<RpcProvider, 
                                                                        const muduo::net::TcpConnectionPtr&,
                                                                        google::protobuf::Message*>
                                                                        (this, &RpcProvider::SendRpcResponse, conn, response);

    // 在框架上根据远端rpc请求，调用相应方法
    service->CallMethod(method, nullptr, request, response, done);
}

void RpcProvider::SendRpcResponse(const muduo::net::TcpConnectionPtr &conn, google::protobuf::Message *response)
{
    // 将response序列化并发送
    std::string response_str;
    if (response->SerializeToString(&response_str))
        conn->send(response_str);
    else
        LOG_ERR("Serialize response error!  %s--%s--%d", __FILE__, __FUNCTION__, __LINE__);
    conn->shutdown();
}