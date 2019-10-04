#include "symbolic.hpp"

#include <sstream>

#include "logger.hpp"
#include "symengine/eval.h"
#include "symengine/parser.h"
#include "symengine/subs.h"
#include "symengine/symengine_exception.h"
#include "symengine/visitor.h"

namespace SymEngine {
void muPrinter::_print_pow(std::ostringstream &o, const RCP<const Basic> &a,
                           const RCP<const Basic> &b) {
  if (eq(*a, *E)) {
    o << "exp(" << apply(b) << ")";
  } else if (eq(*b, *rational(1, 2))) {
    o << "sqrt(" << apply(a) << ")";
  } else {
    o << parenthesizeLE(a, PrecedenceEnum::Pow);
    o << "^";
    o << parenthesizeLE(b, PrecedenceEnum::Pow);
  }
}
}  // namespace SymEngine

namespace symbolic {

std::string Symbolic::toString(
    const SymEngine::RCP<const SymEngine::Basic> &e) const {
  SymEngine::muPrinter p;
  return p.apply(e);
}

Symbolic::Symbolic(const std::vector<std::string> &expressions,
                   const std::vector<std::string> &variables,
                   const std::map<std::string, double> &constants) {
  SPDLOG_DEBUG("parsing {} expressions", expressions.size());
  for (const auto &v : variables) {
    SPDLOG_DEBUG("  - variable {}", v);
    symbols[v] = SymEngine::symbol(v);
    varVec.push_back(symbols.at(v));
  }
  SymEngine::map_basic_basic d;
  for (const auto &p : constants) {
    d[SymEngine::symbol(p.first)] = SymEngine::real_double(p.second);
    SPDLOG_DEBUG("  - constant {} = {}", p.first, p.second);
  }
  for (const auto &expression : expressions) {
    SPDLOG_DEBUG("expr {}", expression);
    expr.push_back(SymEngine::parse(expression)->subs(d));
    SPDLOG_DEBUG("  --> {}", *expr.back());
  }

  // NOTE: don't do symbolic CSE - segfaults!
  lambda.init(varVec, expr, false);
  // compile with LLVM, again no symbolic CSE
  lambdaLLVM.init(varVec, expr, false, 2);
}

std::string Symbolic::simplify(std::size_t i) const {
  return toString(expr.at(i));
}

std::string Symbolic::diff(const std::string &var, std::size_t i) const {
  auto dexpr_dvar = expr.at(i)->diff(symbols.at(var));
  return toString(dexpr_dvar);
}

void Symbolic::relabel(const std::vector<std::string> &newVariables) {
  if (varVec.size() != newVariables.size()) {
    SPDLOG_ERROR(
        "cannot relabel variables: newVariables size {} "
        "does not match number of existing variables {}",
        newVariables.size(), varVec.size());
    return;
  }
  decltype(varVec) newVarVec;
  decltype(symbols) newSymbols;
  SymEngine::map_basic_basic d;
  for (std::size_t i = 0; i < newVariables.size(); ++i) {
    const auto &v = newVariables.at(i);
    newSymbols[v] = SymEngine::symbol(v);
    newVarVec.push_back(newSymbols.at(v));
    d[varVec.at(i)] = newVarVec[i];
    SPDLOG_DEBUG("relabeling {} -> {}", *varVec.at(i), *newVarVec.at(i));
  }
  // substitute new variables into all expressions
  for (auto &e : expr) {
    SPDLOG_DEBUG("expr {}", *e);
    e = e->subs(d);
    SPDLOG_DEBUG("  -> {}", *e);
  }
  // replace old variables with new variables in vector & map
  std::swap(varVec, newVarVec);
  std::swap(symbols, newSymbols);
}

void Symbolic::eval(std::vector<double> &results,
                    const std::vector<double> &vars) {
  lambda.call(results.data(), vars.data());
}

void Symbolic::evalLLVM(std::vector<double> &results,
                        const std::vector<double> &vars) {
  lambdaLLVM.call(results.data(), vars.data());
}

}  // namespace symbolic
