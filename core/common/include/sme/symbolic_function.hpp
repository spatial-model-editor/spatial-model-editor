#pragma once

#include <string>
#include <vector>

namespace sme::common {

/**
 * @brief User-defined function supplied to ``Symbolic`` parsing.
 */
struct SymbolicFunction {
  /**
   * @brief SBML function definition id.
   */
  std::string id;
  /**
   * @brief Function symbol used in expressions.
   */
  std::string name;
  /**
   * @brief Formal function arguments.
   */
  std::vector<std::string> args;
  /**
   * @brief Function body expression.
   */
  std::string body;
};
} // namespace sme::common
