#include "base/threading/platform_thread.h"
#include "base/macros.h"

#include <chrono>
#include <cstdio>
#include <ratio>
#include <thread>

class TrivialThread : public PlatformThread::Delegate {
 public:
  TrivialThread() : run_event_(false) {}

  TrivialThread(const TrivialThread&) = delete;
  TrivialThread& operator=(const TrivialThread&) = delete;

  void ThreadMain() override {
    run_event_ = true;
    uint32_t count = 0;
    char buf[] = "hello thread";
    std::printf("hello thread:%lu\n" , ABSL_ARRAYSIZE(buf));
    while (run_event_) {
      //PlatformThread::Sleep(base::Milliseconds(100));
      if (count++ % 10 == 9) {
        std::printf("hello thread:%d, run time:%d\n" , PlatformThread::CurrentId(), count);
      }

      PlatformThread::Sleep(100);
    }
  }

  void Stop() { run_event_ = false; }

  bool& run_event() { return run_event_; }

 private:
  bool run_event_ { false};
};

void TestPlatformThread(void)
{
  TrivialThread thread;
  PlatformThreadHandle handle;

  PlatformThread::Create(0, &thread, &handle);
  std::this_thread::sleep_for(std::chrono::milliseconds(5000));
  thread.Stop();
  PlatformThread::Join(handle);
}