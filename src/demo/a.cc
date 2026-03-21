#include <algorithm>  // std::transform
#include <iostream>
#include <vector>

struct TData {
  int a;
  char c;
  int b;
};

union UData {
  char a;
  short b;
  char c;
  /* data */
};

template <typename T>
class Base {
 public:
  Base(const T& value);
  ~Base();

 private:
  /* data */
  T data;
};

template <typename T>
Base<T>::Base(const T& value) : data(value) {}

template <typename T>
Base<T>::~Base() {}

#include <bitset>

class TBase {
 public:
  TBase() = default;
  virtual ~TBase() = default;

  virtual void f() { std::cout << "TBase::f()" << std::endl; }
};

#include <stdint.h>

class TDerived : public TBase {
 public:
  TDerived() = default;
  ~TDerived() = default;
  void f() override { std::cout << "TDerived::f()" << std::endl; }

 private:
  uint32_t a;
};

struct TT {
  TT(int val) : data(val) {}

  int operator()(int y) const { return data + y; }

  /* data */
  int data;
};

int main() {
  int value = 0b1010101011011011;
  int out = 0;
  for (int i = 14; i >= 0; i -= 2) {
    int two_bits = (value >> i) & 0x3;  // 获取两个位
    two_bits = ((two_bits & 0x1) << 1) | ((two_bits & 0x2) >> 1);
    out |= (two_bits << i);  // 将交换后的两个位放回原位置
  }

  std::cout << std::bitset<16>(out) << std::endl;
  std::cout << std::endl;
  std::bitset<16> bk = value;
  std::cout << "value: " << bk << std::endl;

  std::cout << "out: " << std::bitset<16>(out) << std::endl;

  std::cout << sizeof(UData) << std::endl;
  std::cout << "Derived:" << sizeof(TDerived) << std::endl;
  return 0;
}