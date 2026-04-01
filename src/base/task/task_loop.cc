
#include "base/task/task_loop.h"

#include <chrono>

namespace base {

TaskLoop::TaskLoop(/* args */) {
  Start();
}

TaskLoop::~TaskLoop() {
  Stop();
}

#define likely(x)   __builtin_expect(!!(x), 1)
#define unlikely(x) __builtin_expect(!!(x), 0)

void TaskLoop::PostTask(const Task& task) {
  {
    std::lock_guard lock(incoming_task_mutex_);
    incoming_task_queue_.push_back(task);
  }
  work_cv_.notify_one();
}

void TaskLoop::Run() {
  for (;;) {
    bool more_work_is_plausible = DoWork();
    if (ShouldQuit()) {
      break;
    }

    if (more_work_is_plausible) {
      continue;
    }

    {
      // waiting for task to arrive
      std::unique_lock lck(work_mutex_);
      work_cv_.wait(lck, [this] { return should_quit_; });
    }
  }
}

bool TaskLoop::DoWork() {
  for (;;) {
    ReloadWorkQueue();
    if (work_queue_.empty())
      break;
    if (ShouldQuit()) {
      return true;
    }

    do {
      Task task = work_queue_.front();
      work_queue_.pop_front();
      if (task.func != nullptr) {
        task.func();
        return true;
      }
      if (ShouldQuit()) {
        return true;
      }
    } while (!work_queue_.empty());
  }

  return false;
}

void TaskLoop::ReloadWorkQueue() {
  work_queue_.MaybeShrinkQueue();
  std::lock_guard lock(incoming_task_mutex_);
  if (!incoming_task_queue_.empty()) {
    incoming_task_queue_.swap(work_queue_);
  }
}

void TaskLoop::Start() {
  should_quit_ = false;

  if (thread_ && thread_->joinable()) {
    return;
  }

  thread_ = std::make_unique<std::thread>(&TaskLoop::Run, this);
}

void TaskLoop::Stop() {
  should_quit_ = true;
  work_cv_.notify_all();  // Wake up the worker thread if it's waiting for a task.

  if (thread_ && thread_->joinable()) {
    thread_->join();
  }
  thread_.reset();
}

void TaskLoop::WaitTaskFinished() {
  // TODO: wait finished
  {
    std::unique_lock<std::mutex> lock(work_mutex_, std::try_to_lock);
    if (lock.owns_lock()) {
    }
  }

  Stop();
}


bool TaskLoop::ShouldQuit() {
  return should_quit_;
}

}  // namespace base