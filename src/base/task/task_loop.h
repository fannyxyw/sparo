#ifndef __BASE_TASK_TASK_LOOP_H__
#define __BASE_TASK_TASK_LOOP_H__

#include <functional>
#include <memory>
#include <mutex>
#include <queue>
#include <thread>
#include <condition_variable>
#include "lazily_deallocated_deque.h"


namespace base {
typedef std::function<void(void)> TaskFunction;

class TaskLoop {
 public:
  TaskLoop(/* args */);
  ~TaskLoop();

  struct Task {
    int32_t a;
    TaskFunction func;
  };

 public:
  void PostTask(const Task& task);
  void ReloadWorkQueue();
  void Start();
  void Stop();
  void WaitTaskFinished();

 private:
  void Run();

  // only run one task at a time,
  // returns true if the task is runned successfully, false the task list is empty.
  bool DoWork();
  bool ShouldQuit();

  using WorkQueue = base::sequence_manager::LazilyDeallocatedDeque<Task>;
 private:
  WorkQueue incoming_task_queue_;
  WorkQueue work_queue_;
  std::mutex incoming_task_mutex_;
  bool should_quit_{false};
  std::unique_ptr<std::thread> thread_;
  std::mutex work_mutex_;
  std::condition_variable work_cv_;
}; // class TaskLoop
}  // namespace base
#endif  // __BASE_TASK_TASK_LOOP_H__
