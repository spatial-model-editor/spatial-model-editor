#include "numerics.hpp"

#define exprtk_disable_comments
#define exprtk_disable_break_continue
#define exprtk_disable_sc_andor
#define exprtk_disable_return_statement
#define exprtk_disable_string_capabilities
#define exprtk_disable_superscalar_unroll
#define exprtk_disable_rtl_io_file
#define exprtk_disable_rtl_vecops
#define exprtk_disable_caseinsensitivity

#ifdef __GNUC__
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wsign-conversion"
#pragma GCC diagnostic ignored "-Wold-style-cast"
#pragma GCC diagnostic ignored "-Wconversion"
#pragma GCC diagnostic ignored "-Wsign-conversion"
#pragma GCC diagnostic ignored "-Wfloat-conversion"
#pragma GCC diagnostic ignored "-Wunused-parameter"
#endif

#include <exprtk/exprtk.hpp>

#ifdef __GNUC__
#pragma GCC diagnostic pop
#endif

#include "logger.hpp"

namespace numerics {

[[noreturn]] static void throwSymbolTableError(
    const exprtk::symbol_table<double> &symbolTable,
    const std::string &symbol) {
  std::string errorMessage = "ExprEval::ExprEval : symbol table error: '";
  errorMessage.append(symbol);
  errorMessage.append("'");
  if (symbolTable.symbol_exists(symbol)) {
    errorMessage.append(" already exists");
  }
  SPDLOG_CRITICAL(errorMessage);
  throw std::invalid_argument(errorMessage);
}

ExprEval::ExprEval(const std::string &expression,
                   const std::vector<std::string> &variableName,
                   std::vector<double> &variableValue,
                   const std::map<std::string, double> &constants)
    : exprtkExpression(std::make_shared<exprtk::expression<double>>()) {
  // construct symbol table of variables and constants
  exprtk::symbol_table<double> exprtkSymbolTable;
  SPDLOG_DEBUG("compiling {}", expression);
  for (std::size_t i = 0; i < variableName.size(); ++i) {
    SPDLOG_TRACE("adding var {}", variableName[i]);
    if (!exprtkSymbolTable.add_variable(variableName[i], variableValue[i])) {
      throwSymbolTableError(exprtkSymbolTable, variableName[i]);
    }
  }
  for (const auto &v : constants) {
    SPDLOG_TRACE("adding constant {} = {}", v.first, v.second);
    if (!exprtkSymbolTable.add_constant(v.first, v.second)) {
      throwSymbolTableError(exprtkSymbolTable, v.first);
    }
  }
  exprtkExpression->register_symbol_table(exprtkSymbolTable);
  // compile expression
  exprtk::parser<double> exprtkParser;
  exprtkParser.settings().disable_all_control_structures();
  exprtkParser.settings().disable_local_vardef();
  exprtkParser.settings().disable_all_logic_ops();
  exprtkParser.settings().disable_all_inequality_ops();
  exprtkParser.settings().disable_all_assignment_ops();
  exprtkParser.dec().collect_variables() = true;
  if (!exprtkParser.compile(expression, *exprtkExpression.get())) {
    std::string errorMessage = "ExprEval::ExprEval : compilation error: ";
    errorMessage.append(exprtkParser.error());
    SPDLOG_CRITICAL(errorMessage);
    throw std::invalid_argument(errorMessage);
  }
  // get list of variables the expression depends on
  exprtk::parser<double>::dependent_entity_collector::symbol_t symbolPair;
  std::deque<exprtk::parser<double>::dependent_entity_collector::symbol_t>
      symbolPairList;
  exprtkParser.dec().symbols(symbolPairList);
  std::string strListSymbols = "";
  for (const auto &pair : symbolPairList) {
    if (pair.second == exprtk::parser<double>::e_st_variable) {
      std::string str = pair.first;
      const auto iter = constants.find(pair.first);
      if (iter != constants.cend()) {
        str.append(" (");
        str.append(std::to_string(iter->second));
        str.append(")");
      }
      str.append(", ");
      strListSymbols.append(str);
    }
  }
  SPDLOG_DEBUG("  - symbols used: {}", strListSymbols);
}

double ExprEval::operator()() const { return exprtkExpression->value(); }

std::vector<std::string> getSymbols(const std::string &expression) {
  std::vector<std::string> symbols;
  exprtk::collect_variables(expression, symbols);
  return symbols;
}

}  // namespace numerics
