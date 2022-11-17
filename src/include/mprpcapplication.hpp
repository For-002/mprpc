#pragma once

#include"mprpcconfig.hpp"
#include<google/protobuf/service.h>
#include"mprpcchannel.hpp"
#include"mprpccontroller.hpp"

// mprpc框架类
class MprpcApplication
{
public:
    // 初始化mprpc框架
    static void Init(int argc, char** argv);

    // 获取单例对象
    static MprpcApplication& GetInstance();

    // 获取 MprpcConfig 对象
    static MprpcConfig GetConfig();
private:
    static MprpcConfig m_config;

    MprpcApplication();
    MprpcApplication(const MprpcApplication&) = delete;
    MprpcApplication(MprpcApplication&&) = delete;
};
