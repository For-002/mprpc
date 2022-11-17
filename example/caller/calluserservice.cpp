#include"mprpcapplication.hpp"
#include"user.pb.h"
#include"logger.hpp"
#include<string>

int main(int argc, char **argv)
{
    // 程序要使用 mprpc 框架享受rpc服务，必须先调用框架的初始化函数
    MprpcApplication::Init(argc, argv);

    fixbug::UserServiceRpc_Stub stub(new MprpcChannel());

    // 创建 rpc 方法的request和response
    fixbug::LoginRequest request;
    request.set_name("zhangsan");
    request.set_pwd("123");
    fixbug::LoginResponse response;

    // 创建 MprpcController 对象
    MprpcController controller = MprpcController();

    // 发起rpc方法调用
    stub.Login(&controller, &request, &response, nullptr);

    if (controller.Failed())
    {
        std::cout << controller.ErrorText() << std::endl;
        //LOG_ERR("%s", controller.ErrorText().c_str());
    }
    else
    {
        if (0 != response.result().errcode())
            std::cout << "Rpc login response error: " << response.result().errmsg() << std::endl;
        
        std::cout << "Rpc login success: " << response.success() << std::endl;
    }
    return 0;
}