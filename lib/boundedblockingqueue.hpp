#ifndef BOUNDEDBLOCKINGQUEUE_HPP
#define BOUNDEDBLOCKINGQUEUE_HPP

#include <queue>
#include <memory>
#include <condition_variable>
#include <exception>
#include <mutex>

template<typename T>
class BoundedBlockingQueue
{
public:
	// 构造函数，默认队列容量为1024, 禁止隐式构造
	explicit BoundedBlockingQueue(int capacity = 1024);
	~BoundedBlockingQueue() = default;

	// 出队，返回值时当前出队的元素的指针(shared_ptr)
	std::shared_ptr<T> dequeue();

	// 入队,将newValue添加进队列
	void enqueue(T newValue);
	// 判空
	bool empty() const;
	// 获取队列大小
	int size() const;
private:
	int m_capacity;
	// 互斥锁
	mutable std::mutex m_mutex;
	// 队列结构
	std::queue<T> m_safequeue;
	// 条件变量，用于唤醒enqueue或dequeue
	std::condition_variable m_notEmptyCond;
	std::condition_variable m_notFullCond;
};

// 构造函数，初始化size为0, capacity
template<typename T>
BoundedBlockingQueue<T>::BoundedBlockingQueue(int capacity) : m_capacity(capacity)
{

}

template<typename T>
bool BoundedBlockingQueue<T>::empty() const
{
	// 获取锁阻塞出入队列操作
	std::lock_guard<std::mutex> lock(m_mutex);
	return m_safequeue.empty();
}

template<typename T>
int BoundedBlockingQueue<T>::size() const
{
	// 获取锁阻塞出入队列操作
	std::lock_guard<std::mutex> lock(m_mutex);
	return m_safequeue.size();
}

template<typename T>
void BoundedBlockingQueue<T>::enqueue(T newValue)
{

	{
		// 上锁，防止同时入队
		std::unique_lock<std::mutex> lock(m_mutex);
		// 使用条件变量，未满时直接enqueue，如果队列已满则阻塞在此,等价于while(m_safequeue.size() == m_capacity) m_notFullCond.wait(lock)
		m_notFullCond.wait(lock, [this] {return m_safequeue.size() < m_capacity; });
		// 入队
		m_safequeue.push(std::move(newValue));
	}// lock析构解锁

	// 此时enqueue了一个,队列必然不为空，条件变量通知可以被pop()
	m_notEmptyCond.notify_one();
}

template<typename T>
std::shared_ptr<T> BoundedBlockingQueue<T>::dequeue()
{
	// 与enqueue类似
	std::unique_lock<std::mutex> lock(m_mutex);
	// 条件变量,不为空时继续运行,等价于while(m_safequeue.empty()) m_notFullCond.wait(lock)
	m_notEmptyCond.wait(lock, [this] {return !m_safequeue.empty(); });

	// 获取队首
	std::shared_ptr<T> res(std::make_shared<T>(std::move(m_safequeue.front())));
	// 出队
	m_safequeue.pop();
	lock.unlock(); // pop操作已经完成,可以直接解锁不用等待整个函数结束析构自行解锁
	// 出队后必然队列不满，条件变量通知可以被push()
	m_notFullCond.notify_one();
	return res;
};

#endif // BOUNDEDBLOCKINGQUEUE_HPP
