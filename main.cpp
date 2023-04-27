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