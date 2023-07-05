#include <chrono>
#include <iostream>
#include <vector>

#include "thread_pool.h"

int main() {
  sparo::ThreadPool pool(8, "sparo.worker");
  std::vector<std::future<int> > results;

  for (int i = 0; i < 8; ++i) {
    results.emplace_back(pool.Enqueue([i] {
      std::cout << "hello " << i << std::endl;
      std::this_thread::sleep_for(std::chrono::seconds(1));
      std::cout << "world " << i << std::endl;
      return i * i;
    }));
  }

  for (auto&& result : results) {
    std::cout << result.get() << ' ';
  }
  std::cout << std::endl;

  return 0;
}
