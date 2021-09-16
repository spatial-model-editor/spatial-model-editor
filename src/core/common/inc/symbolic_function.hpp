#pragma once

#include <string>
#include <vector>

namespace sme::common {

struct SymbolicFunction {
  std::string id;
  std::string name;
  std::vector<std::string> args;
  std::string body;
};
} // namespace sme::common
