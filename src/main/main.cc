
#include <assert.h>
#include <optional>
#include <unistd.h>
#include <chrono>
#include <iostream>
#include <mutex>

#include "base/command_line.h"
#include "base/logging.h"
#include "base/task/lazily_deallocated_deque.h"
#include "base/containers/circular_deque.h"
#include "third-party/base/singleton.h"
#include "third-party/threadpool.h"
#include "base/thread_pool.h"
#include "tsl/robin_map.h"
#include "base/containers/flat_map.h"

// Round up |size| to a multiple of alignment, which must be a power of two.
template <typename T, typename = std::enable_if_t<std::is_integral_v<T>>>
constexpr T AlignUp(T size, T alignment) {
  return (size + alignment - 1) & ~(alignment - 1);
}

int tasks = 0, done = 0, fail = 0;

std::timed_mutex gMutex;

void RunTaskLoopTest();

class FooClass {
public:
  static FooClass* GetInstance() {
   return base::Singleton<FooClass>::get();
  }

   void foo() {
    printf("hello singleton class\n");
   }
 };

#define THREAD 32
#define QUEUE 512

void TestJson();
void TestMinHeap();
void TestPlatformThread();

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

using namespace std;

/* 记忆化搜索 */
int dfs(int i, vector<int>& mem) {
  // 已知 dp[1] 和 dp[2] ，返回之
  if (i == 1 || i == 2)
    return i;
  // 若存在记录 dp[i] ，则直接返回之
  if (mem[i] != -1)
    return mem[i];
  // dp[i] = dp[i-1] + dp[i-2]
  int count = dfs(i - 1, mem) + dfs(i - 2, mem);
  // 记录 dp[i]
  mem[i] = count;
  return count;
}

/* 爬楼梯：记忆化搜索 */
int climbingStairsDFSMem(int n) {
  // mem[i] 记录爬到第 i 阶的方案总数，-1 代表无记录
  vector<int> mem(n + 1, -1);
  return dfs(n, mem);
}
int climbingStairsDP(int n) {
  if (n == 1 || n == 2)
    return n;
  // 初始化 dp 表，用于存储子问题的解
  vector<int> dp(n + 1);
  // 初始状态：预设最小子问题的解
  dp[1] = 1;
  dp[2] = 2;
  // 状态转移：从较小子问题逐步求解较大子问题
  for (int i = 3; i <= n; i++) {
    dp[i] = dp[i - 1] + dp[i - 2];
  }
  return dp[n];
}

void backtrack(vector<int>& choices, int state, int n, vector<int>& res) {
  // 当爬到第 n 阶时，方案数量加 1
  if (state == n)
    res[0]++;
  // 遍历所有选择
  for (auto& choice : choices) {
    // 剪枝：不允许越过第 n 阶
    if (state + choice > n)
      continue;
    // 尝试：做出选择，更新状态
    backtrack(choices, state + choice, n, res);
    // 回退
  }
}

/* 爬楼梯：回溯 */
int climbingStairsBacktrack(int n) {
  vector<int> choices = {1, 2};  // 可选择向上爬 1 阶或 2 阶
  int state = 0;                 // 从第 0 阶开始爬
  vector<int> res = {0};         // 使用 res[0] 记录方案数量
  backtrack(choices, state, n, res);
  return res[0];
}

/* 记忆化搜索 */
int dfsMem(int i, vector<int>& mem) {
  // 已知 dp[1] 和 dp[2] ，返回之
  if (i == 0)
    return 1;
  // 若存在记录 dp[i] ，则直接返回之
  if (mem[i] != -1)
    return mem[i];
  int dsa = 1;
  // dp[i] = dp[i-1] + dp[i-2]
  for (int n = 1; n < i + 1; n++) {
    /* code */
    dsa = n * dsa;
  }

  return dsa;
}

/* 爬楼梯：记忆化搜索 */
int FACDFSMem(int n) {
  // mem[i] 记录爬到第 i 阶的方案总数，-1 代表无记录
  vector<int> mem(n + 1, -1);
  return dfsMem(n, mem);
}

int main(int argc, char* argv[]) {
  base::circular_deque<int> deque;
  deque.push_back(1);
  deque.push_back(1);
  deque.push_back(1);
  deque.push_back(1);
  base::CommandLine cmd(argc, argv);
  if (cmd.HasSwitch("version")) {
    LOGD(cmd.GetSwitchValueASCII("version").data());
  } else {
    LOGD("version");
  }

  FooClass::GetInstance()->foo();
  // auto ii = std::atoi(argv[1]);
  // printf("%d,,===%d \n", FACDFSMem(ii), ii);
  TestMinHeap();

  // test for robin_map
  base::flat_map<int, int> aa;
  aa.emplace(2, 9);
  aa.emplace(1, 2);
  aa.emplace(std::make_pair(23, 9));
  std::cout << "size:" << aa.size() << std::endl;
  std::cout << aa[23] << std::endl;
  std::printf("hello thread pool\n");

  // std::cout << std::pow(0, std::atoi(argv[1])) << std::endl;

  TestJson();

  TestPlatformThread();

  std::cout << argv[0] << std::endl;
  base::sequence_manager::LazilyDeallocatedDeque<int32_t> incoming_queue;
  base::sequence_manager::LazilyDeallocatedDeque<int32_t> work_queue;
  incoming_queue.push_back(12);
  {
    work_queue.MaybeShrinkQueue();
    std::mutex mutex_queue;
    std::unique_lock<std::mutex> lock(mutex_queue);
    work_queue.swap(incoming_queue);
  }

  std::cout << "capacity:" << incoming_queue.capacity() << std::endl;
  std::cout << "capacity align::" << AlignUp(12, 4096) << std::endl;

  std::cout << "capacity:" << incoming_queue.capacity() << std::endl;

  incoming_queue.push_back(12);

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

  RunTaskLoopTest();

  return 0;
}
