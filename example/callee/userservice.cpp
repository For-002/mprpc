#include<iostream>
#include<string>
#include"user.pb.h"
#include"mprpcapplication.hpp"
#include"rpcprovider.hpp"
#include"logger.hpp"

class UserService : public fixbug::UserServiceRpc
{
public:
    bool Login(std::string name, std::string pwd)
    {
        std::cout << "Doing local service: Login" << std::endl;
        std::cout << "name: " << name << " pwd: " << pwd << std::endl;
        return true;
    }

    void Login(::google::protobuf::RpcController* controller,
                       const ::fixbug::LoginRequest* request,
                       ::fixbug::LoginResponse* response,
                       ::google::protobuf::Closure* done)
    {
        // 框架给业务上报了请求参数LoginRequest，应用获取相应数据做本地业务
        std::string name = request->name();
        std::string pwd = request->pwd();

        // 做本地业务
        bool login_result = this->Login(name, pwd);

        // 将业务处理结果写入响应，包括错误码，错误消息，返回值
        fixbug::ResultCode *code = response->mutable_result();
        code->set_errcode(0);
        code->set_errmsg("");
        response->set_success(login_result);

        // 执行回调操作，执行响应对象数据的序列号和网络发送的工作由框架完成
        done->Run();
    }
};

int main(int argc, char** argv)
{
    // 初始化mprpc框架
    MprpcApplication::Init(argc, argv);

    // 创建一个rpc网络服务对象，把UserService对象发布到rpc节点上
    RpcProvider provider;
    provider.NotifyService(new UserService());

    // 启动一个rpc服务发布节点，Run以后，进入阻塞状态，等待远程的rpc调用请求
    provider.Run();

    return 0;
}