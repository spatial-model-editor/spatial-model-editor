#include "symbolic.hpp"

#include <sstream>

#include "symengine/eval.h"
#include "symengine/parser.h"
#include "symengine/subs.h"
#include "symengine/symengine_exception.h"
#include "symengine/visitor.h"

#include "logger.hpp"

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

  // don't do symbolic CSE - segfaults!
  lambda.init(varVec, expr, false);
  // compile with LLVM, again no symbolic CSE
  lambdaLLVM.init(varVec, expr, false, 2);
}  // namespace symbolic

std::string Symbolic::simplify(std::size_t i) const {
  return toString(expr.at(i));
}

std::string Symbolic::diff(const std::string &var, std::size_t i) const {
  auto dexpr_dvar = expr.at(i)->diff(symbols.at(var));
  return toString(dexpr_dvar);
}

void Symbolic::eval(std::vector<double> &results,
                    const std::vector<double> &vars) {
  //  SPDLOG_WARN("{} vars: {}", vars.size(), vars);
  //  SPDLOG_WARN("{} results before: {}", results.size(), results);
  lambda.call(results.data(), vars.data());
  //  SPDLOG_WARN("results after: {}", results);
}

void Symbolic::evalLLVM(std::vector<double> &results,
                        const std::vector<double> &vars) {
  lambdaLLVM.call(results.data(), vars.data());
}

}  // namespace symbolic
