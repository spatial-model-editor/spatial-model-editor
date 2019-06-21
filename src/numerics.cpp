#include "numerics.h"

namespace numerics {

ExprEval::ExprEval(const std::string &expression,
                   const std::vector<std::string> &variableName,
                   std::vector<double> &variableValue,
                   const std::vector<std::string> &constantName,
                   const std::vector<double> &constantValue) {
  exprtk::symbol_table<double> exprtkSymbolTable;
  for (std::size_t i = 0; i < variableName.size(); ++i) {
    exprtkSymbolTable.add_variable(variableName[i], variableValue[i]);
  }
  for (std::size_t i = 0; i < constantName.size(); ++i) {
    exprtkSymbolTable.add_constant(constantName[i], constantValue[i]);
  }
  exprtkExpression.register_symbol_table(exprtkSymbolTable);
  exprtk::parser<double> exprtkParser;
  exprtkParser.compile(expression, exprtkExpression);
}

double ExprEval::operator()() const { return exprtkExpression.value(); }

}  // namespace numerics
