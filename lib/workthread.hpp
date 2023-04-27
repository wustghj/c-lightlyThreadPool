#ifndef WORKTHREAD_HPP
#define WORKTHREAD_HPP

#include "boundedblockingqueue.hpp"
#include <chrono>
#include <functional>
#include <atomic>
#include <memory>
#include <thread>

using std::thread;

class WorkThread
{
public:
    WorkThread(BoundedBlockingQueue<std::function<void()>> &tasks) : m_tasks(tasks)
                                               , m_finish(false)
                                               , m_state(1)
    {
        // 创建线程
        m_thread = thread([this]()
        {
            // 不关闭的状态时循环运行
            while (!m_finish)
            {
                // 默认位阻塞状态
                m_state = STATE_WAIT;
                
                // 进程未结束并且工作队列未空时，阻塞
                while (!m_finish && m_tasks.empty())
                {
                    // 休眠50毫秒，而不是死循环占用支援
                    std::this_thread::sleep_for(std::chrono::milliseconds(50));
                }

                if (m_finish)
                {
                    break;
                }

                auto task = std::move(m_tasks.dequeue());
                if(task != nullptr)
                {
                    // 此时将状态设置为工作中
                    m_state = STATE_WORK;
                    // 运行任务
                    (*task.get())();
                }
            }
        });
    }

    ~WorkThread()
    {
        if (m_thread.joinable())
            m_thread.join();
    }

    void finish()
    {
        m_finish = true;
    }

    int getState() const
    {
        return m_state;
    }

    thread::id getId()
    {
        return m_thread.get_id();
    }

    // 获取当前线程的引用
    thread& getCurrentThread()
    {
        return m_thread;
    }

    constexpr static int STATE_WAIT = 1;
    constexpr static int STATE_WORK = 2;
    constexpr static int STATE_EXIT = 3;
    
private:
    // 工作队列
    BoundedBlockingQueue<std::function<void()>> &m_tasks;
    
    std::atomic_bool m_finish;

    // 当前状态
    std::atomic_int m_state;
    // 当前线程
    thread m_thread;
};

using ThreadPtr = std::shared_ptr<WorkThread> ;

#endif // WORKTHREAD_HPP