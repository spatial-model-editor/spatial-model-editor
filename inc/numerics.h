// Math expression evaluator
//  - takes a math expression as a string
//  - also a list of constants and variables
//  - compiles the mathematical expression using exprtk
//  - stores the variables as references (i.e. if they are later changed the
//  expression when evaluated will use the new values)
//  - evaluates the expression

#pragma once

#include <string>

// disable unused exprtk features: reduces build time and executable size
#define exprtk_disable_break_continue
#define exprtk_disable_sc_andor
#define exprtk_disable_return_statement
#define exprtk_disable_enhanced_features
#define exprtk_disable_string_capabilities
#define exprtk_disable_superscalar_unroll
#define exprtk_disable_rtl_io_file
#define exprtk_disable_rtl_vecops
#define exprtk_disable_caseinsensitivity
#include "exprtk.hpp"

namespace numerics {

class ReactionEvaluate {
 public:
  // the variables are stored as references, so their values can be changed
  // and the expression can then be re-evaluated without re-compiling
  ReactionEvaluate(const std::string &expression,
                   const std::vector<std::string> &variableName,
                   std::vector<double> &variableValue,
                   const std::vector<std::string> &constantName,
                   const std::vector<double> &constantValue);
  // evaluate compiled expression
  double operator()();

 private:
  exprtk::symbol_table<double> exprtkSymbolTable;
  exprtk::expression<double> exprtkExpression;
};

}  // namespace numerics
