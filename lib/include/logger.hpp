#pragma once

#include"lockqueue.hpp"
#include<iostream>
#include<string>
#include<stdio.h>

enum LogLevel
{
    INFO,
    ERROR,
};

// mprpc 框架的日志系统
class Logger
{
public:
    // 获取日志的单例
    static Logger& GetInstance();
    // 设置日志级别
    void SetLogLevel(LogLevel);
    // 写队列，只负责把日志信息写入 LockQueue 缓冲区中
    void Log(std::string msg);
private:
    int m_logLevel;
    LockQueue<std::string> m_lockQue;

    Logger();
    Logger(const Logger&) = delete;
    Logger(Logger&&) = delete;
};

#define LOG_INFO(logmsgformat, ...) \
    do \
    {  \
        Logger &logger = Logger::GetInstance(); \
        logger.SetLogLevel(INFO); \
        char c[2048] = {0}; \
        snprintf(c, 2048, logmsgformat, ##__VA_ARGS__); \
        logger.Log(c); \
    } while (0);


#define LOG_ERR(logmsgformat, ...) \
    do \
    {  \
        Logger &logger = Logger::GetInstance(); \
        logger.SetLogLevel(ERROR); \
        char c[2048] = {0}; \
        snprintf(c, 2048, logmsgformat, ##__VA_ARGS__); \
        logger.Log(c); \
    } while (0);
