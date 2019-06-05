#ifndef NUMERICS_H
#define NUMERICS_H

#include <string>

// disable unused features: reduces build time and executable size
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
  // compile the given string containing a mathematical expression using exprtk
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

#endif  // NUMERICS_H
