#include "numerics.h"

namespace numerics {

ReactionEvaluate::ReactionEvaluate(const std::string &expression,
                                   const std::vector<std::string> &speciesName,
                                   std::vector<double> &speciesValue,
                                   const std::vector<std::string> &constantName,
                                   const std::vector<double> &constantValue) {
  for (std::size_t i = 0; i < speciesName.size(); ++i) {
    exprtkSymbolTable.add_variable(speciesName[i], speciesValue[i]);
  }
  for (std::size_t i = 0; i < constantName.size(); ++i) {
    exprtkSymbolTable.add_constant(constantName[i], constantValue[i]);
  }
  exprtkExpression.register_symbol_table(exprtkSymbolTable);
  exprtk::parser<double> exprtkParser;
  exprtkParser.compile(expression, exprtkExpression);
}

double ReactionEvaluate::operator()() { return exprtkExpression.value(); }

}  // namespace numerics
