#include"mprpcconfig.hpp"
#include<iostream>
#include<string>

void MprpcConfig::ReadConfigFile(const char *config_file)
{
    FILE *fp = fopen(config_file, "r");
    if (nullptr == fp)
    {
        std::cout << config_file << " is not exist" << std::endl;
        exit(EXIT_FAILURE);
    }

    while (!feof(fp))
    {
        char buf[512] = {0};
        fgets(buf, 512, fp);

        // 转换成 string 便于处理
        std::string str_buf = buf;
        
        Trim(str_buf);
        
        if (str_buf[0] == '#' || str_buf.empty())
            continue;

        int idx = str_buf.find('=');
        if (-1 == idx)  // 没有 = ，配置项不合法
            continue;
        
        std::string key;
        std::string value;
        key = str_buf.substr(0, idx);
        Trim(key);
        int endidx = str_buf.find('\n', idx);
        value = str_buf.substr(idx + 1, endidx - idx - 1);
        Trim(value);

        m_configMap.insert({key, value});
    }
}

std::string MprpcConfig::LoadConfig(std::string item)
{
    auto iter = m_configMap.find(item);

    if (iter == m_configMap.end())
        return "";
    else
        return iter->second;
}

void MprpcConfig::Trim(std::string &str)
{
    int idx = str.find_first_not_of(' ');
    if (-1 != idx)  // 确实存在前置空格，要删掉
        str = str.substr(idx, str.size() - idx);
    
    idx = str.find_last_not_of(' ');
    if (-1 != idx)  // 确实存在后置空格，要删掉
        str = str.substr(0, idx + 1);
}