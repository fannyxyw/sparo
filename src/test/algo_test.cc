#include <list>
#include <utility>
#include <vector>

void BubbleSort(std::vector<int>& numbs) {
  std::list<std::pair<int, int>> list;
  for(size_t i = numbs.size() - 1; i > 0; --i) {
    bool flag = false;
    for (size_t j = 0; j < i; ++j) {
      if (numbs[j] > numbs[i]) {
        std::swap(numbs[i], numbs[j]);
        flag = true;
      }
    }

    if (flag) {
      break;
    }
  }
}
