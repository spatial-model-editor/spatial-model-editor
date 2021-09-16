// SimpleSymbolic
//  - basic symbolic algebra functionality
//  - lightweight alternative to Symbolic

#pragma once

#include <cstddef>
#include <string>
#include <utility>
#include <vector>

namespace sme::common {

class SimpleSymbolic {
public:
  static std::string divide(const std::string &expr, const std::string &var);
  static std::string multiply(const std::string &expr, const std::string &var);
  static std::string
  substitute(const std::string &expr,
             const std::vector<std::pair<std::string, double>> &constants);
  static bool contains(const std::string &expr, const std::string &var);
};

} // namespace sme::common
