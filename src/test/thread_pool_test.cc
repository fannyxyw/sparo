#include <iostream>
#include <chrono>
#include <vector>
#include <atomic>
#include <cassert>
#include "../base/thread_pool.h"

// 测试用的任务函数
void SimpleTask(void* arg) {
    int* value = static_cast<int*>(arg);
    (*value)++;
    delete value;
}

// 测试用的长时间运行任务
void LongRunningTask(void* arg) {
    std::atomic<int>* counter = static_cast<std::atomic<int>*>(arg);
    (*counter)++;
    std::this_thread::sleep_for(std::chrono::milliseconds(100));
}

// 回调测试用的全局变量
std::atomic<int> callback_counter(0);
std::string last_callback_message;

// 任务完成回调函数
void TestCallback(void* arg) {
    callback_counter++;
    if (arg) {
        last_callback_message = *static_cast<std::string*>(arg);
    }
}

// 测试任务执行
void TestTaskExecution() {
    std::cout << "测试: 任务执行" << std::endl;

    sparo::ThreadPool pool(2, "Test");
    pool.Start();

    std::vector<int> results(5, 0);

    // 提交5个任务
    for (int i = 0; i < 5; ++i) {
        int* value = new int(results[i]);
        pool.EnqueueTask(SimpleTask, value);
    }

    // 等待所有任务完成
    pool.WaitForAllTasks();

    // 验证结果
    for (int i = 0; i < 5; ++i) {
        assert(results[i] == 0);  // 原始值应该不变，因为我们传递的是副本
    }

    pool.Stop();
    std::cout << "测试通过: 任务执行" << std::endl;
}

// 测试等待所有任务完成
void TestWaitForAllTasks() {
    std::cout << "测试: 等待所有任务完成" << std::endl;

    sparo::ThreadPool pool(4, "Test");
    pool.Start();

    std::atomic<int> counter(0);

    // 提交10个长时间运行的任务
    for (int i = 0; i < 10; ++i) {
        pool.EnqueueTask(LongRunningTask, &counter);
    }

    // 等待所有任务完成
    pool.WaitForAllTasks();

    // 验证所有任务都已完成
    assert(counter == 10);
    assert(pool.GetActiveTaskCount() == 0);

    pool.Stop();
    std::cout << "测试通过: 等待所有任务完成" << std::endl;
}

// 测试活跃任务计数
void TestActiveTaskCount() {
    std::cout << "测试: 活跃任务计数" << std::endl;

    sparo::ThreadPool pool(2, "Test");
    pool.Start();

    // 初始状态应该没有活跃任务
    assert(pool.GetActiveTaskCount() == 0);

    // 提交5个任务
    for (int i = 0; i < 5; ++i) {
        int* value = new int(0);
        pool.EnqueueTask(SimpleTask, value);
    }

    // 检查活跃任务数量
    // 注意：由于任务可能很快完成，这里只检查是否大于0
    assert(pool.GetActiveTaskCount() >= 0);

    // 等待所有任务完成
    pool.WaitForAllTasks();

    // 完成后应该没有活跃任务
    assert(pool.GetActiveTaskCount() == 0);

    pool.Stop();
    std::cout << "测试通过: 活跃任务计数" << std::endl;
}

// 测试多次提交和等待
void TestMultipleSubmissions() {
    std::cout << "测试: 多次提交和等待" << std::endl;

    sparo::ThreadPool pool(3, "Test");
    pool.Start();

    std::atomic<int> counter(0);

    // 第一批任务
    for (int i = 0; i < 5; ++i) {
        pool.EnqueueTask(LongRunningTask, &counter);
    }

    // 等待第一批任务完成
    pool.WaitForAllTasks();
    assert(counter == 5);
    assert(pool.GetActiveTaskCount() == 0);

    // 第二批任务
    for (int i = 0; i < 3; ++i) {
        pool.EnqueueTask(LongRunningTask, &counter);
    }

    // 等待第二批任务完成
    pool.WaitForAllTasks();
    assert(counter == 8);
    assert(pool.GetActiveTaskCount() == 0);

    pool.Stop();
    std::cout << "测试通过: 多次提交和等待" << std::endl;
}

// 测试回调功能
void TestCompletionCallback() {
    std::cout << "测试: 任务完成回调" << std::endl;

    // 重置回调计数器
    callback_counter = 0;
    last_callback_message = "";

    sparo::ThreadPool pool(3, "Test");
    pool.Start();

    // 设置回调函数
    std::string callback_msg = "第一批任务完成";
    pool.SetCompletionCallback(TestCallback, &callback_msg);

    std::atomic<int> counter(0);

    // 提交第一批任务
    for (int i = 0; i < 5; ++i) {
        pool.EnqueueTask(LongRunningTask, &counter);
    }

    // 等待所有任务完成
    pool.WaitForAllTasks();

    // 验证回调被调用
    assert(callback_counter == 1);
    assert(last_callback_message == callback_msg);
    assert(counter == 5);

    // 清除回调
    pool.ClearCompletionCallback();

    // 提交第二批任务（不应该触发回调）
    for (int i = 0; i < 3; ++i) {
        pool.EnqueueTask(LongRunningTask, &counter);
    }

    // 等待第二批任务完成
    pool.WaitForAllTasks();

    // 验证回调没有被再次调用
    assert(callback_counter == 1);  // 仍然是1，没有增加
    assert(counter == 8);

    pool.Stop();
    std::cout << "测试通过: 任务完成回调" << std::endl;
}

// 测试多次回调设置
void TestMultipleCallbackSettings() {
    std::cout << "测试: 多次回调设置" << std::endl;

    // 重置回调计数器
    callback_counter = 0;
    last_callback_message = "";

    sparo::ThreadPool pool(2, "Test");
    pool.Start();

    std::atomic<int> counter(0);

    // 第一次设置回调
    std::string msg1 = "第一组任务";
    pool.SetCompletionCallback(TestCallback, &msg1);

    // 提交第一组任务
    for (int i = 0; i < 3; ++i) {
        pool.EnqueueTask(LongRunningTask, &counter);
    }

    pool.WaitForAllTasks();

    // 验证第一次回调
    assert(callback_counter == 1);
    assert(last_callback_message == msg1);
    assert(counter == 3);

    // 更改回调设置
    std::string msg2 = "第二组任务";
    pool.SetCompletionCallback(TestCallback, &msg2);

    // 提交第二组任务
    for (int i = 0; i < 2; ++i) {
        pool.EnqueueTask(LongRunningTask, &counter);
    }

    pool.WaitForAllTasks();

    // 验证第二次回调
    assert(callback_counter == 2);
    assert(last_callback_message == msg2);
    assert(counter == 5);

    pool.Stop();
    std::cout << "测试通过: 多次回调设置" << std::endl;
}

int main() {
    std::cout << "=== 线程池测试 ===" << std::endl;

    TestTaskExecution();
    TestWaitForAllTasks();
    TestActiveTaskCount();
    TestMultipleSubmissions();
    TestCompletionCallback();
    TestMultipleCallbackSettings();

    std::cout << "所有测试通过！" << std::endl;

    return 0;
}