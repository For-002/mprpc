#include"logger.hpp"
#include<time.h>

Logger& Logger::GetInstance()
{
    static Logger logger;
    return logger;
}

Logger::Logger()
{
    // 启动专门的写日志线程
    std::thread writeLogTask([&](){
        for (;;)
        {
            // 获取当前日期，然后取日志信息，写入相应的日志文件中 a+
            time_t now = time(nullptr);
            tm *nowtm = localtime(&now);

            char file_name[128] = {0};
            sprintf(file_name, "./log/%d-%d-%d-log.txt", nowtm->tm_year + 1900, nowtm->tm_mon + 1, nowtm->tm_mday);

            FILE *pf = fopen(file_name, "a+");
            if (nullptr == pf)
            {
                std::cout << "Open log file: " << file_name << " error!" << std::endl;
                exit(EXIT_FAILURE);
            }
            
            char time_buf[128] = {0};
            sprintf(time_buf, "%02d:%02d:%02d: => %-9s",
                        nowtm->tm_hour,
                        nowtm->tm_min,
                        nowtm->tm_sec,
                        (m_logLevel == INFO ? "[INFO]" : "[ERROR]"));

            std::string msg = m_lockQue.Pop();
            msg.insert(0, time_buf);
            msg.append("\n");
            fputs(msg.c_str(), pf);
            fclose(pf);
        }
    });
    // 设置分离线程（守护线程），主线程结束后，detach的线程不会结束，而是做完自己的逻辑之后才结束
    writeLogTask.detach();
}

void Logger::Log(std::string msg)
{
    m_lockQue.Push(msg);
}

void Logger::SetLogLevel(LogLevel level)
{
    m_logLevel = level;
}