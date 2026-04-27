#include <iostream>
#include <chrono>
#include <random>
#include "../base/thread_pool.h"

// 任务函数示例
void TaskFunction(void* arg) {
    int* taskId = static_cast<int*>(arg);
    std::cout << "任务 " << *taskId << " 开始执行，线程ID: " << std::this_thread::get_id() << std::endl;

    // 模拟任务执行时间
    std::random_device rd;
    std::mt19937 gen(rd());
    std::uniform_int_distribution<> dis(500, 2000);
    std::this_thread::sleep_for(std::chrono::milliseconds(dis(gen)));

    std::cout << "任务 " << *taskId << " 执行完成" << std::endl;
    delete taskId;  // 释放内存
}

// 任务完成回调函数
void AllTasksCompletedCallback(void* arg) {
    std::cout << "*** 所有任务已完成！回调函数被调用 ***" << std::endl;

    // 可以通过arg参数传递额外信息
    if (arg) {
        std::string* message = static_cast<std::string*>(arg);
        std::cout << "回调消息: " << *message << std::endl;
    }
}

int main() {
    std::cout << "=== 线程池示例 ===" << std::endl;

    // 创建线程池，包含4个工作线程
    sparo::ThreadPool pool(4, "Worker");

    // 启动线程池
    pool.Start();

    // 设置任务完成回调
    std::string callbackMessage = "这是一组任务完成的回调通知";
    pool.SetCompletionCallback(AllTasksCompletedCallback, &callbackMessage);

    // 提交10个任务
    std::cout << "提交10个任务到线程池..." << std::endl;
    for (int i = 1; i <= 10; ++i) {
        int* taskId = new int(i);  // 为每个任务创建一个ID
        pool.EnqueueTask(TaskFunction, taskId);
    }

    // 检查活跃任务数量
    std::cout << "当前活跃任务数量: " << pool.GetActiveTaskCount() << std::endl;

    // 等待所有任务完成
    std::cout << "等待所有任务完成..." << std::endl;
    pool.WaitForAllTasks();

    // 再次检查活跃任务数量
    std::cout << "所有任务完成后，活跃任务数量: " << pool.GetActiveTaskCount() << std::endl;

    // 清除回调函数
    pool.ClearCompletionCallback();

    // 提交第二批任务（这次不会有回调）
    std::cout << "\n提交第二批任务（无回调）..." << std::endl;
    for (int i = 11; i <= 15; ++i) {
        int* taskId = new int(i);
        pool.EnqueueTask(TaskFunction, taskId);
    }

    // 等待第二批任务完成
    pool.WaitForAllTasks();
    std::cout << "第二批任务完成" << std::endl;

    // 停止线程池
    std::cout << "停止线程池..." << std::endl;
    pool.Stop();

    std::cout << "示例结束" << std::endl;
    return 0;
}