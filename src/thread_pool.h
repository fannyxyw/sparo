#ifndef THREAD_POOL_H
#define THREAD_POOL_H

#include <sys/prctl.h>
#include <condition_variable>
#include <functional>
#include <future>
#include <memory>
#include <mutex>
#include <queue>
#include <string>
#include <thread>
#include <vector>

/**
 * From https://github.com/progschj/ThreadPool/blob/master/ThreadPool.h
 */

namespace sparo {

class ThreadPool {
 public:
  ThreadPool(size_t, const std::string& thread_name);
  ~ThreadPool();

  void Quit();

  template <class F, class... Args>
  auto Enqueue(F&& f, Args&&... args)
      -> std::future<typename std::invoke_result<F, Args...>::type>;

 private:
  // launch all worker threads.
  void Start();

  // work thread main
  void WorkerThreadMain();

 private:
  size_t threads_;
  std::string thread_name_;
  // need to keep track of threads so we can join them
  std::vector<std::thread> workers_;

  // the task queue
  std::queue<std::function<void()>> tasks_;

  // synchronization
  std::mutex queue_mutex_;
  std::condition_variable condition_;
  bool should_quit_{false};
};

// the constructor just launches some amount of workers
inline ThreadPool::ThreadPool(size_t threads, const std::string& thread_name)
    : threads_(threads), thread_name_(thread_name) {
  Start();
}

inline void ThreadPool::Start() {
  for (size_t i = 0; i < threads_; ++i) {
    workers_.emplace_back([this, i] {
      std::string name = (thread_name_.length() == 0) ? "worker" : thread_name_;
      name = name + "." + std::to_string(i);
      prctl(PR_SET_NAME, name.c_str());

      WorkerThreadMain();
    });
  }
}

void ThreadPool::WorkerThreadMain() {
  for (;;) {
    std::function<void()> task;
    {
      std::unique_lock<std::mutex> lock(queue_mutex_);
      condition_.wait(lock, [this] { return should_quit_ || !tasks_.empty(); });
      if (should_quit_ && tasks_.empty()) {
        return;
      }
      task = std::move(tasks_.front());
      tasks_.pop();
    }
    task();
  }
}

// add new work item to the pool
template <class F, class... Args>
auto ThreadPool::Enqueue(F&& f, Args&&... args)
    -> std::future<typename std::invoke_result<F, Args...>::type> {
  using return_type = typename std::invoke_result<F, Args...>::type;

  auto task = std::make_shared<std::packaged_task<return_type()>>(
      std::bind(std::forward<F>(f), std::forward<Args>(args)...));

  std::future<return_type> res = task->get_future();
  {
    std::unique_lock<std::mutex> lock(queue_mutex_);
    if (should_quit_) {
      return res;
    }
    tasks_.emplace([task]() { (*task)(); });
  }
  condition_.notify_one();
  return res;
}

void inline ThreadPool::Quit() {
  {
    std::unique_lock<std::mutex> lock(queue_mutex_);
    should_quit_ = true;
  }

  condition_.notify_all();

  for (std::thread& worker : workers_) {
    if (worker.joinable()) {
      worker.join();
    }
  }
}

// the destructor joins all threads
inline ThreadPool::~ThreadPool() {
  Quit();
}

}  // namespace sparo

#endif
