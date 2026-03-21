#include <iostream>
#include <vector>
#include <algorithm>

using namespace std;

int minStairDP(const vector <int>& costs) {
  int n = costs.size() - 1;
  if (n == 0)
    return 0;
  if ((n == 1) || (n == 2))
    return costs[n];

  int first = costs[1];
  int second = costs[2];
  int temp;
  for (size_t i = 3; i <= n; i++) {
    temp = second;
    second = min(first, second) + costs[i];
    first = temp;
  }
  return second;
}

int main() {
  vector<int> costs = {0, 1, 10, 1, 1, 1, 10, 1, 1, 10, 11, 10, 1, 1, 1, 10, 1, 1, 10, 1};

  int res = minStairDP(costs);
  cout << "爬完楼梯的最低代价为 " << res << endl;
}
