// Math expression evaluator (simple wrapper around ExprTk)
//   - takes a math expression as a string
//   - also a list of constants and variables
//   - compiles the mathematical expression using exprtk
//   - stores the variables as references (i.e. if they are later changed the
//     expression when evaluated will use the new values)
//   - evaluates the expression

#pragma once

#include <map>
#include <memory>
#include <string>
#include <vector>

// forward declaration to avoid exposing exprtk header
namespace exprtk {
template <typename T>
class expression;
}

namespace numerics {

class ExprEval {
 public:
  // the variables are stored as references, so their values can be changed
  // and the expression can then be re-evaluated without re-compiling
  ExprEval(const std::string &expression,
           const std::vector<std::string> &variableName,
           std::vector<double> &variableValue,
           const std::map<std::string, double> &constants);
  // evaluate compiled expression
  double operator()() const;

 private:
  // store pointer to exprtk::expression as the type is not yet defined
  std::shared_ptr<exprtk::expression<double>> exprtkExpression;
};

// return a vector of all symbols in the expression
std::vector<std::string> getSymbols(const std::string &expression);

}  // namespace numerics
