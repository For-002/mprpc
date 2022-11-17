#pragma once

#include<unordered_map>
#include<string>

class MprpcConfig
{
public:
    // 读取配置文件
    void ReadConfigFile(const char *config_file);

    // 查询相应的配置信息
    std::string LoadConfig(std::string item);
private:
    // 去除字符串前置和后置的空格
    void Trim(std::string &str);
    
    // 注：虽说 stl 容器是线程不安全的，但是当 rpc 框架启动只需要 Init 一次，m_config 只会被写一次，所以这里根本不用考虑线程安全的问题
    std::unordered_map<std::string, std::string> m_configMap;
};