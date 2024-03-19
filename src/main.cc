
#include <assert.h>
#include <unistd.h>
#include <chrono>
#include <iostream>
#include <mutex>

#include "lazily_deallocated_deque.h"
#include "third-party/threadpool.h"
#include "thread_pool.h"
#include "tsl/robin_map.h"
#include "third-party/base/singleton.h"

// Round up |size| to a multiple of alignment, which must be a power of two.
template <typename T, typename = std::enable_if_t<std::is_integral_v<T>>>
constexpr T AlignUp(T size, T alignment) {
  return (size + alignment - 1) & ~(alignment - 1);
}

int tasks = 0, done = 0, fail = 0;


std::timed_mutex gMutex;

#define THREAD 32
#define QUEUE 512

void TestJson();

void dummy_task(void* arg) {
  std::unique_lock<std::timed_mutex> kk(gMutex, std::defer_lock);
  if (kk.try_lock_for(std::chrono::milliseconds(1))) {
    usleep(10000);
    done++;
    // fprintf(stdout, "Done %d tasks\n", done);
  } else {
    ++fail;
  }
}

int main(int argc, char* argv[]) {
  // test for robin_map
  tsl::robin_map<int, int> aa;
  aa.emplace(1, 2);
  aa.emplace(std::make_pair(23, 9));
  std::cout << "size:" << aa.size() << std::endl;
  std::cout << aa[23] << std::endl;

  TestJson();
  std::cout << argv[0] << std::endl;
  base::sequence_manager::LazilyDeallocatedDeque<int32_t> dque;
  dque.push_back(12);

  std::cout << "capacity:" << dque.capacity() << std::endl;
  std::cout << "capacity align::" << AlignUp(12, 4096) << std::endl;
  dque.push_back(12);

  threadpool_t* pool{nullptr};

  assert((pool = threadpool_create(THREAD, QUEUE, 0)) != NULL);
  fprintf(stderr,
          "Pool started with %d threads and "
          "queue size of %d\n",
          THREAD, QUEUE);

  while (threadpool_enqueue(pool, &dummy_task, NULL, 0) == 0) {
    tasks++;
  }

  fprintf(stderr, "Added %d tasks\n", tasks);
  while ((tasks / 2) > (done + fail)) {
    usleep(10000);
  }
  assert(threadpool_destroy(pool, 0) == 0);
  fprintf(stderr, "Did %d tasks\n", done);

  auto last_done = done;
  sparo::ThreadPool tp(4, "");
  tp.Start();
  constexpr static int32_t task_num = 6;
  for (int32_t i = 0; i < task_num; i++) {
    tp.EnqueueTask(dummy_task, nullptr);
  }

  std::this_thread::sleep_for(std::chrono::microseconds(100));
  tp.Stop();
  fprintf(stderr, "c++ 11 done %d tasks\n", done - last_done);

  return 0;
}
