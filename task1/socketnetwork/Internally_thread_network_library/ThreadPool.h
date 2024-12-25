// 线程池头文件
#include <iostream>
#include <vector>
#include <thread>
#include <queue>
#include <functional>
#include <mutex>
#include <condition_variable>
#include <atomic>
#include <exception>

// 线程池类
class ThreadPool {
private:
    std::vector<std::thread> workers;               // 工作线程
    std::queue<std::function<void()>> tasks;        // 任务队列
    std::mutex queueMutex;                          // 保护任务队列的互斥锁，确保在一个线程访问队列时，其他线程必须等待该线程释放锁后才能访问
    std::condition_variable condition;              // 用于线程间的同步，通知工作线程队列中有新任务或线程池需要停止
    std::atomic<bool> stop;                         // 线程池停止标志，使用 std::atomic 确保对 stop 的修改和读取操作是线程安全的，不需要额外加锁

public:
    ThreadPool() : stop(false) {}
    // 初始化线程池
    bool init(int32_t threadCount) {
        if (threadCount <= 0 || threadCount > 16) {
            return false;
        }
        // 逐个创建 threadCount 个工作线程，一开始任务队列为空的情况下，工作线程会阻塞在 condition.wait(lock) 处
        for (int i = 0; i < threadCount; i++) {
            std::cout << "Create thread " << i << " success..." << std::endl;
            workers.emplace_back([this]() {
                while (true) {
                    // 该线程从任务队列中接收的任务
                    std::function<void()> task;
                    {
                        // 创建一个对象lock，并且在构造函数中自动对queueMutex加锁
                        std::unique_lock<std::mutex> lock(queueMutex);
                        // 释放 lock(queueMutex) 互斥锁 , [this](){}是一个lambda表达式，表示线程被唤醒的条件，线程被唤醒后重新加锁
                        condition.wait(lock, [this]() {
                            return stop || !tasks.empty();  // 当任务队列为空且线程池设置为未停止时，等待
                        });

                        // 线程池停止且任务队列为空，退出线程
                        if (stop && tasks.empty())
                            return; // 退出线程
                        
                        // 任务队列非空，该线程取出任务执行
                        task = std::move(tasks.front());
                        tasks.pop();
                        // 花括号结束时，lock对象被销毁，queueMutex解锁，其他线程获取任务队列的访问权限
                    }
                    // 执行任务
                    task();
                }
            });
        }
        return true;
    }

    // 向任务队列中添加任务
    void enqueueTask(const std::function<void()>& task) {
        {
            std::lock_guard<std::mutex> lock(queueMutex);
            tasks.push(task);
        }
        // 任务队列中有新任务，通知一个工作线程唤醒
        // 如果没有线程在等待，通知会被“丢弃”，即当前没有线程会被唤醒，但这不影响程序正常执行
        condition.notify_one();
    }

    // 主线程停止线程池并清理资源
    void shutdown() {
        {
            std::lock_guard<std::mutex> lock(queueMutex);
            stop = true;
        }
        // 停止线程池，通知所有工作线程，工作线程是逐个地退出的
        // 如果此时任务队列还有未取出的任务，工作线程会继续从任务队列中取出剩余的任务并执行，但此时 stop 为 true，执行完最后的任务后，工作线程会退出
        condition.notify_all();
        for (std::thread& worker : workers) {
            if (worker.joinable())
                worker.join();
        }
        std:: cout << "ThreadPool has shuted down..." << std::endl;
    }

    ~ThreadPool() {
        shutdown();
    }
};