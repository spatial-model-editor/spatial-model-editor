// Math expression evaluator (simple wrapper around ExprTk)
//  - takes a math expression as a string
//  - also a list of constants and variables
//  - compiles the mathematical expression using exprtk
//  - stores the variables as references (i.e. if they are later changed the
//  expression when evaluated will use the new values)
//  - evaluates the expression

#pragma once

#include <string>

// disable unused/undesired exprtk features:
//  - reduces build time and executable size
//  - don't parse undesired language features
#define exprtk_disable_comments
#define exprtk_disable_break_continue
#define exprtk_disable_sc_andor
#define exprtk_disable_return_statement
// note: disabling enhanced_features gives:
//  - smaller executable
//  - faster compilation
//  - but slower evaluation of expressions
#define exprtk_disable_enhanced_features
#define exprtk_disable_string_capabilities
#define exprtk_disable_superscalar_unroll
#define exprtk_disable_rtl_io_file
#define exprtk_disable_rtl_vecops
#define exprtk_disable_caseinsensitivity
#include "exprtk.hpp"

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
  exprtk::expression<double> exprtkExpression;
};

// return a vector of all symbols in the expression
std::vector<std::string> getSymbols(const std::string &expression);

}  // namespace numerics
