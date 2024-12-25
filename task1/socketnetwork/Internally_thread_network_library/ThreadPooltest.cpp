#include "ThreadPool.h"
#include <iostream>
#include <chrono>

// 一个简单的任务函数，打印任务的 ID 和线程 ID
void exampleTask(int taskID) {
    std::cout << "Task " << taskID << " is being executed by thread "
              << std::this_thread::get_id() << std::endl;
    // 模拟任务执行时间
    std::this_thread::sleep_for(std::chrono::milliseconds(100));
}

int main() {
    const int threadCount = 4;      // 线程池中的线程数
    const int taskCount = 10;       // 总共要执行的任务数

    std::cout << "Initializing ThreadPool with " << threadCount << " threads..." << std::endl;

    // 创建线程池实例
    ThreadPool pool;
    if (!pool.init(threadCount)) {
        std::cerr << "Failed to initialize ThreadPool!" << std::endl;
        return -1;
    }

    std::cout << "ThreadPool initialized successfully!" << std::endl;

    // 向线程池中添加任务
    for (int i = 1; i <= taskCount; ++i) {
        pool.enqueueTask([i]() {
            exampleTask(i);
        });
        std::cout << "Task " << i << " has been added to the queue." << std::endl;
    }

    std::cout << "All tasks have been added to the queue. Waiting for tasks to complete..." << std::endl;

    // 等待一段时间，确保所有任务执行完成
    std::this_thread::sleep_for(std::chrono::seconds(2));
    return 0;
}