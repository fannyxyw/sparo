#ifndef __THREAD_POOL_H_
#define __THREAD_POOL_H_ 1

#include <sys/prctl.h>
#include <condition_variable>
#include <mutex>
#include <queue>
#include <string>
#include <thread>
#include <vector>
#include <atomic>

namespace sparo {

class ThreadPool {
 public:
  ThreadPool(int32_t num, const std::string& name = "");
  ~ThreadPool();

 public:
  typedef void (*TaskFunction)(void*);
  typedef void (*CallbackFunction)(void*);  // 回调函数类型定义

  struct Task {
    TaskFunction function;
    void* argument;
  };

  void Start(void);
  void Stop(void);
  bool EnqueueTask(TaskFunction function, void* argument);

  // 等待所有任务完成
  void WaitForAllTasks(void);

  // 获取当前活跃任务数量
  size_t GetActiveTaskCount(void) const;

  // 设置所有任务完成时的回调函数
  void SetCompletionCallback(CallbackFunction callback, void* arg = nullptr);

  // 清除回调函数
  void ClearCompletionCallback();

 private:
  void WorkerMain(int32_t index);
  bool PeekTask(Task& task);
  void ScheduleWork(void) {
    condition_.notify_one();
  }

  void ScheduleQuit(void) {
    should_quit_ = true;
    condition_.notify_all();
  }

 private:
  int32_t thread_num_{4};
  std::string thread_name_;
  std::vector<std::thread> workers_;
  std::queue<Task> tasks_;
  bool should_quit_{false};

  std::mutex mutex_;
  std::condition_variable condition_;
  std::condition_variable finished_;  // 用于等待所有任务完成
  std::atomic<size_t> active_tasks_{0};  // 活跃任务计数

  // 回调相关
  CallbackFunction completion_callback_{nullptr};
  void* callback_arg_{nullptr};
};

ThreadPool::ThreadPool(int32_t num, const std::string& name)
    : thread_num_(num), thread_name_(name) {}

void ThreadPool::Start(void) {
  should_quit_ = false;
  for (int32_t index = 0; index < thread_num_; ++index) {
    workers_.emplace_back([this, index] {
      std::string name = thread_name_.empty() ? "worker" : thread_name_;
      name = name + "." + std::to_string(index);
      prctl(PR_SET_NAME, name.c_str());

      WorkerMain(index);
    });
  }
}

void ThreadPool::WorkerMain(int32_t index) {
  for (;;) {
     if (should_quit_) {
      break;
    }

    {
      std::unique_lock<std::mutex> lock(mutex_);
      condition_.wait_for(lock, std::chrono::milliseconds(200),
          [this] { return should_quit_ || !tasks_.empty(); });
    }

    Task task;
    if (!PeekTask(task)) {
      continue;
    }

    if (should_quit_) {
      break;
    }

    task.function(task.argument);

    // 任务完成后减少活跃任务计数
    active_tasks_--;
    if (active_tasks_ == 0) {
      std::unique_lock<std::mutex> lock(mutex_);
      finished_.notify_all();  // 通知所有等待的线程

      // 如果设置了回调函数，调用它
      if (completion_callback_) {
        completion_callback_(callback_arg_);
      }
    }
  }
}

bool ThreadPool::PeekTask(Task& task) {
  std::unique_lock<std::mutex> lock(mutex_);
  if (tasks_.empty()) {
    return false;
  }
  task = std::move(tasks_.front());
  tasks_.pop();
  return true;
}

bool ThreadPool::EnqueueTask(TaskFunction func, void* argument) {
  {
    std::unique_lock<std::mutex> lock(mutex_);
    Task task = {.function = func, .argument = argument};
    tasks_.push(task);
    active_tasks_++;  // 增加活跃任务计数
  }
  ScheduleWork();
  return true;
}

void ThreadPool::Stop(void) {
  should_quit_ = true;
  ScheduleQuit();
  for (auto& worker : workers_) {
    if (worker.joinable()) {
      worker.join();
    }
  }
}

void ThreadPool::WaitForAllTasks(void) {
  std::unique_lock<std::mutex> lock(mutex_);
  finished_.wait(lock, [this] { return active_tasks_ == 0; });
}

size_t ThreadPool::GetActiveTaskCount(void) const {
  return active_tasks_.load();
}

void ThreadPool::SetCompletionCallback(CallbackFunction callback, void* arg) {
  std::unique_lock<std::mutex> lock(mutex_);
  completion_callback_ = callback;
  callback_arg_ = arg;
}

void ThreadPool::ClearCompletionCallback() {
  std::unique_lock<std::mutex> lock(mutex_);
  completion_callback_ = nullptr;
  callback_arg_ = nullptr;
}

ThreadPool::~ThreadPool() {
  Stop();
}

}  // namespace sparo

#endif  // __THREAD_POOL_H_