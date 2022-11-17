#include"mprpcchannel.hpp"
#include"google/protobuf/descriptor.h"
#include"rpcheader.pb.h"
#include<sys/socket.h>
#include<sys/types.h>
#include<errno.h>
#include<arpa/inet.h>
#include<netinet/in.h>
#include<unistd.h>
#include"mprpcapplication.hpp"
#include"mprpccontroller.hpp"
#include"logger.hpp"
#include"zookeeperutil.hpp"

// 所有通过stub代理对象调用的rpc方法，最终都会来到这里，统一做rpc方法调用的数据序列化和网络发送
//    header_size + service_name + method_name + args_size + args
void MprpcChannel::CallMethod(const google::protobuf::MethodDescriptor* method,
                                    google::protobuf::RpcController* controller,
                                    const google::protobuf::Message* request,
                                    google::protobuf::Message* response, 
                                    google::protobuf::Closure* done)
{
    // 获取 rpc 请求的服务名和服务的方法名
    const google::protobuf::ServiceDescriptor *service = method->service();
    std::string service_name = service->name();
    std::string method_name = method->name();

    // 将 rpc 请求的参数序列化为字符流
    std::string args_str;
    if (!request->SerializeToString(&args_str))
    {
        controller->SetFailed("Serialize request error!");
        //LOG_ERR("Serialize request error!  %s--%s--%d", __FILE__, __FUNCTION__, __LINE__);
        return ;
    }

    // 填写 rpcheader 的信息
    mprpc::RpcHeader rpc_header;
    rpc_header.set_service_name(service_name);
    rpc_header.set_method_name(method_name);
    rpc_header.set_args_size(args_str.size());

    // 将 rpcheader 部分序列化为字符流
    std::string rpc_header_str;
    if (!rpc_header.SerializeToString(&rpc_header_str))
    {
        controller->SetFailed("Serialize rpc_header error!");
        //LOG_ERR("Serialize rpc_header error!  %s--%s--%d", __FILE__, __FUNCTION__, __LINE__);
        return ;
    }

    // 组织待发送的 rpc 请求字符流
    std::string send_rpc_str;
    uint32_t header_size = rpc_header_str.size();
    send_rpc_str.insert(0, std::string((char*)&header_size, 4));

    send_rpc_str += rpc_header_str;
    send_rpc_str += args_str;

    // 准备将序列化好的数据发送出去
    int clientfd = socket(AF_INET, SOCK_STREAM, 0);
    if (-1 == clientfd)
    {
        char errbuf[512] = {0};
        sprintf(errbuf, "Create socket error! errno: %d", errno);
        controller->SetFailed(errbuf);
        //LOG_ERR("Create socket error! errno: %d  %s--%s--%d", errno, __FILE__, __FUNCTION__, __LINE__);
        return ;
    }

    // 向rpc服务器查询提供目标方法的主机的ip和端口号
    std::string method_path = "/" + service_name + "/" + method_name;
    ZkClient zkcli;
    zkcli.Start();
    std::string addr = zkcli.GetData(method_path.c_str());
    if ("" == addr)
    {
        controller->SetFailed(method_path + " is not exist!");
        //LOG_ERR("%s is not exist!", method_path.c_str());
        return ;
    }
    int idx = addr.find(":");
    if (-1 == idx)
    {
        controller->SetFailed("Address:" + addr + " is invalid!");
        //LOG_ERR("Address:%s is inbalid!", addr.c_str());
        return ;
    }
    std::string ip = addr.substr(0, idx);
    uint16_t port = atoi(addr.substr(idx + 1, addr.size() - idx - 1).c_str());

    struct sockaddr_in server_address;
    server_address.sin_family = AF_INET;
    server_address.sin_port = htons(port);
    server_address.sin_addr.s_addr = inet_addr(ip.c_str());
    
    // 连接 rpc 节点
    if (-1 == connect(clientfd, (struct sockaddr*)&server_address, sizeof(server_address)))
    {
        char errbuf[512] = {0};
        sprintf(errbuf, "Connect error! errno: %d", errno);
        controller->SetFailed(errbuf);
        //LOG_ERR("Connect error! errno: %d  %s--%s--%d", errno, __FILE__, __FUNCTION__, __LINE__);
        return ;
    }

    // 发送 rpc 请求
    if (-1 == send(clientfd, send_rpc_str.c_str(), send_rpc_str.size(), 0))
    {
        char errbuf[512] = {0};
        sprintf(errbuf, "Send error! errno: %d", errno);
        controller->SetFailed(errbuf);
        close(clientfd);
        //LOG_ERR("Send error! errno: %d  %s--%s--%d", errno, __FILE__, __FUNCTION__, __LINE__);
        return ;
    }

    // 接收 rpc 响应
    char recv_buf[1024] = {0};
    int recv_size = 0;
    if (-1 == (recv_size = recv(clientfd, recv_buf, 1024, 0)))
    {
        char errbuf[512] = {0};
        sprintf(errbuf, "Recv error! errno: %d", errno);
        controller->SetFailed(errbuf);
        close(clientfd);
        //LOG_ERR("Recv error! errno: %d  %s--%s--%d", errno, __FILE__, __FUNCTION__, __LINE__);
        return ;
    }
    
    // 将 rpc 响应反序列化并写入 response
    //    这里不能采用 使用recv_buf构造一个string然后再反序列化string的策略，因为在构造时，遇到"\0"时，就停止构造了，会损失很多数据
    if (!response->ParseFromArray(recv_buf, recv_size))
    {
        char errbuf[2048] = {0};
        sprintf(errbuf, "Parse from string error! Response_str: %s", recv_buf);
        controller->SetFailed(errbuf);
        close(clientfd);
        //LOG_ERR("Parse from string error! Response_str: %s  %s--%s--%d", recv_buf, __FILE__, __FUNCTION__, __LINE__);
        return ;
    }
}