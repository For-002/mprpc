#pragma once

#include<thread>
#include<queue>
#include<mutex>
#include<condition_variable>

template<typename T>
class LockQueue
{
public:
    // 向队列写入，各个epoll线程都可能会调用
    void Push(const T &data)
    {
        std::unique_lock<std::mutex> lock(m_mutex);
        m_queue.push(data);
        m_condvariable.notify_one();
        lock.unlock();
    }

    // 从队列读出，只有写日志文件会调用
    T Pop()
    {
        std::unique_lock<std::mutex> mtx(m_mutex);
        if (m_queue.empty())
        {
            // 队列为空，调用wait等待写线程写入
            m_condvariable.wait(mtx);
        }
        // 拿到了锁，可以读了
        T t = m_queue.front();
        m_queue.pop();
        return t;
    }
private:
    std::mutex m_mutex;
    std::queue<T> m_queue;
    std::condition_variable m_condvariable;
};