- [Mprpc简介](#mprpc简介)
- [预编译环境](#预编译环境)
- [编译](#编译)
- [框架的使用说明](#框架的使用说明)
  - [编写proto文件](#编写proto文件)
  - [服务提供方](#服务提供方)
  - [服务使用方](#服务使用方)

# Mprpc简介

设计了一个基于muduo网络库的rpc分布式微服务框架，对请求和响应进行关联，支持较大吞吐量场景下的同步请求发送

# 预编译环境

安装好下面的环境和库

* 安装CMake编译环境
* 安装protobuf
* 安装zookeeper
* 安装muduo网络库

# 编译

运行项目根目录下的脚本文件`auto_build.sh`(需要授予可执行权限)即可自动完成编译。完成编译后，框架的所有头文件和静态库都存放在`lib`目录下。

# 框架的使用说明

## 编写proto文件

发布一个rpc服务必须先编写proto文件。

proto文件内需要包含对rpc服务所有方法的描述，以及方法涉及的参数和返回值类型的描述。然后使用protoc生成对应的头文件和源文件。

例如现在要发布一个用户登录的方法。需要编写如下的proto文件：

```protobuf
syntax = "proto3";

package fixbug;

option cc_generic_services = true;

message ResultCode
{
    int32 errcode = 1;
    bytes errmsg = 2;
}

message LoginRequest
{
    bytes name = 1;
    bytes pwd = 2;
}

message LoginResponse
{
    ResultCode result = 1;
    bool success = 2;
}

service UserServiceRpc
{
    rpc Login(LoginRequest) returns(LoginResponse);
}
```

假设该文件名为`user.proto`，使用protoc通过该文件会生成`user.pb.cc`和`user.pb.h`文件。

## 服务提供方

* 编写`.conf`配置文件，包含zookeeper server的ip和端口号。从而能够发布rpc方法。
* 定义本地的方法实现。
* 继承`.pb.h`头文件中生成的类，重写它的`virtual`方法。

------

接上面的例子，`.pb.h`文件中生成的类定义如下：

```c++
class UserServiceRpc : public ::PROTOBUF_NAMESPACE_ID::Service {
 protected:
  // This class should be treated as an abstract interface.
  inline UserServiceRpc() {};
 public:
  virtual ~UserServiceRpc();

  typedef UserServiceRpc_Stub Stub;

  static const ::PROTOBUF_NAMESPACE_ID::ServiceDescriptor* descriptor();

  virtual void Login(::PROTOBUF_NAMESPACE_ID::RpcController* controller,
                       const ::fixbug::LoginRequest* request,
                       ::fixbug::LoginResponse* response,
                       ::google::protobuf::Closure* done);

  // implements Service ----------------------------------------------

  const ::PROTOBUF_NAMESPACE_ID::ServiceDescriptor* GetDescriptor();
  void CallMethod(const ::PROTOBUF_NAMESPACE_ID::MethodDescriptor* method,
                  ::PROTOBUF_NAMESPACE_ID::RpcController* controller,
                  const ::PROTOBUF_NAMESPACE_ID::Message* request,
                  ::PROTOBUF_NAMESPACE_ID::Message* response,
                  ::google::protobuf::Closure* done);
  const ::PROTOBUF_NAMESPACE_ID::Message& GetRequestPrototype(
    const ::PROTOBUF_NAMESPACE_ID::MethodDescriptor* method) const;
  const ::PROTOBUF_NAMESPACE_ID::Message& GetResponsePrototype(
    const ::PROTOBUF_NAMESPACE_ID::MethodDescriptor* method) const;

 private:
  GOOGLE_DISALLOW_EVIL_CONSTRUCTORS(UserServiceRpc);
};
```

继承`UserServiceRpc`类，重写`Login`方法，代码如下：

```C++
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
        // 从 request 中获取方法参数
        std::string name = request->name();
        std::string pwd = request->pwd();

        // 调用本地方法
        bool login_result = this->Login(name, pwd);

        // 将本地方法的返回结果存入 response
        fixbug::ResultCode *code = response->mutable_result();
        code->set_errcode(0);
        code->set_errmsg("");
        response->set_success(login_result);

        // 执行回调操作
        done->Run();
    }
};
```

重写方法时要完成四件事：

* 从 `request` 中获取方法参数
* 调用本地方法
* 将本地方法的返回结果存入 `response`
* 执行回调操作

然后就可以在`main`函数中发布该rpc服务了。实例代码如下：

```c++
#include<iostream>
#include<string>
#include"user.pb.h"
#include"mprpcapplication.hpp"
#include"rpcprovider.hpp"
#include"logger.hpp"

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
```



## 服务使用方

服务使用方需编写`.conf`配置文件，配置zookeeper server的ip和端口号，从而能够发现rpc方法。然后初始化框架，创建Stub对象，并填写`request`，创建`response`，将他们传入Stub对象的相应方法。最后就能从`response`中取出远程rpc方法的返回值。

接上面的例子，示例代码如下：

```c++
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
```

