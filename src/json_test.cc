#include <iostream>
#include <string>

#include "third-party/json.hpp"

void TestJson() {
  using Json = nlohmann::json;

  std::string str = "{\"a\":1, \"b\":1}";
  auto root = Json::parse(str, nullptr, false, false);

  std::cout << root["a"] << std::endl;
  std::cout << root.dump() << std::endl;
}