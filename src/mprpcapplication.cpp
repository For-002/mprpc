#include"mprpcapplication.hpp"
#include<iostream>
#include<unistd.h>

MprpcConfig MprpcApplication::m_config;

void ShowArgHelp()
{
    std::cout << "correct format: command -i <configfile>" << std::endl;
}

MprpcApplication::MprpcApplication()
{
    
}

void MprpcApplication::Init(int argc, char** argv)
{
    if (argc < 3)
    {
        ShowArgHelp();
        exit(EXIT_FAILURE);
    }

    char ch = 0;
    std::string config_file;
    while ((ch = getopt(argc, argv, "i:")) != -1)
    {
        switch(ch)
        {
            case 'i':
                config_file = optarg;
                break;
            case '?':   // 出现了未定义的选项
                ShowArgHelp();
                exit(EXIT_FAILURE);
            case ':':   // 选项正确，但是没有参数
                ShowArgHelp();
                exit(EXIT_FAILURE);
        }
    }

    m_config.ReadConfigFile(config_file.c_str());
}

MprpcApplication& MprpcApplication::GetInstance()
{
    static MprpcApplication app;
    return app;
}

MprpcConfig MprpcApplication::GetConfig()
{
    return m_config;
}