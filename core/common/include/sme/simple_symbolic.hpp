// SimpleSymbolic
//  - basic symbolic algebra functionality
//  - lightweight alternative to Symbolic

#pragma once

#include <cstddef>
#include <set>
#include <string>
#include <utility>
#include <vector>

namespace sme::common {

/**
 * @brief Lightweight symbolic string transformations.
 *
 * Provides simple expression manipulation without full parser/LLVM compile
 * support from ``Symbolic``.
 */
class SimpleSymbolic {
public:
  /**
   * @brief Divide expression ``expr`` by symbol ``var``.
   */
  static std::string divide(const std::string &expr, const std::string &var);
  /**
   * @brief Multiply expression ``expr`` by symbol ``var``.
   */
  static std::string multiply(const std::string &expr, const std::string &var);
  /**
   * @brief Substitute constants into an expression string.
   */
  static std::string
  substitute(const std::string &expr,
             const std::vector<std::pair<std::string, double>> &constants);
  /**
   * @brief Check whether ``expr`` contains ``var``.
   */
  static bool contains(const std::string &expr, const std::string &var);
  /**
   * @brief Extract symbol names referenced by ``expr``.
   */
  static std::set<std::string, std::less<>> symbols(const std::string &expr);
};

} // namespace sme::common
