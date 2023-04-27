### 一个轻量的C++线程池库

c++ STL 并未自带一个线程池库，因此想借此机会写一个线程池模板，以后想用线程池直接导入

优点

* 使用有限阻塞队列，而不是在线程池里使用互斥锁，性能好
* 使用管理线程，在任务繁忙时自动扩充线程池，在任务空闲时减小线程池，合理分配资源

使用方法，将lib目录直接复制到项目地址

然后在想使用线程池的块中导入threadpool.hpp即可

先获取线程池实例

auto threadpool = ThreadPool::instance();

在addTask()这个API中直接加入你想添加到线程池中处理的函数即可

**例**

```cpp
#include "lib/threadpool.hpp"
#include <iostream>

void output()
{
    std::cout<<"func output() : null\n";
}

void sum(int a, int b)
{
    std::cout<<"func sum() :"<<a+b<<std::endl;
}

int main()
{
    auto threadpool = ThreadPool::instance();

    threadpool->addTask(output);
    // 用bind将参数打包
    threadpool->addTask(std::bind(&sum, 5 , 8));

    return 0;
}
```

输出

```
func output() : null
func sum() :13
```

