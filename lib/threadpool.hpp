#ifndef THREADPOOL_HPP
#define THREADPOOL_HPP

#include <thread>
#include <chrono>
#include <atomic>
#include <functional>
#include <memory>
#include <unordered_map>

// 并发安全的的有限阻塞队列
#include "boundedblockingqueue.hpp"
#include "workthread.hpp"

using std::thread;

// 获取cpu核心数
static const int coreNum = std::thread::hardware_concurrency();

class ThreadPool
{
public:
    // 核心,将任务添加到线程池,加入任务队列
    template <typename F>
    void addTask(F &&task)
    {
        // 完美转发
        m_tasks.enqueue(std::forward<F>(task));
    }

    void finish()
    {
        m_finish = true;
        for (auto &it:m_thread)
        {
            if (it.second->getState() == WorkThread::STATE_WAIT)
                it.second->finish();
        }
    }

    // 单例模式获取
    static ThreadPool *instance()
    {
        static ThreadPool ins;
        return &ins;
    }

private:
    // 增加一个工作线程
    void addThread()
    {
        auto tidPtr = std::make_shared<WorkThread>(m_tasks);

        m_thread[tidPtr->getId()] = tidPtr;
    }

    // 删除所有wait状态的工作线程
    void delThread()
    {
        for (auto &it : m_thread)
        {
            if (it.second->getState() == WorkThread::STATE_WAIT)
            {
                it.second->finish();

                // 在哈希表中删除该线程信息
                m_thread.erase(it.first);
            }
        }
    }

    // 核心
    ThreadPool(int min = (coreNum >> 1), int max = (coreNum << 1))
        : m_min(min), m_max(max), m_finish(false)
    {
        // 先创建最小量线程
        for (int i = 0; i < m_min; i++)
        {
            addThread();
        }
        // 利用管理线程
        m_manageThread = thread([this]() 
        {
            int cnt = 0;
            while (!m_finish)
            {
                // 如果当前线程池太小
                if (m_tasks.size() > 2 * m_thread.size() && m_thread.size() < m_max)
                {
                    addThread();
                }
                else
                {
                    cnt = 0;
                    for (auto &it:m_thread)
                    {
                        if (it.second->getState() == WorkThread::STATE_WAIT)
                            cnt++;
                    }

                    if (cnt > 2 * m_tasks.size() && m_thread.size() > m_min)
                    {
                        delThread();
                    }
                }
                std::this_thread::sleep_for(std::chrono::seconds(1));
            }
        });
    };

    ~ThreadPool()
    {
        if (m_manageThread.joinable())
            m_manageThread.join();
    }

    int m_min;
    int m_max;
    // 销毁线程池flag
    std::atomic_bool m_finish;
    // 工作队列

    BoundedBlockingQueue<std::function<void()>> m_tasks;
    // 存储线程信息
    std::unordered_map<thread::id, ThreadPtr> m_thread;
    // 管理者线程
    thread m_manageThread;
};

#endif // THREADPOOL_H