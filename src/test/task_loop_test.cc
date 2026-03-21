#include "base/task/task_loop.h"
#include "stdio.h"

void RunTaskLoopTest() {
  auto foo = []() {
    printf("foo test\n");
  };

  base::TaskLoop loop;
  loop.PostTask(base::TaskLoop::Task{1, foo});
  std::this_thread::sleep_for(std::chrono::milliseconds(20));
  loop.Stop();
}
