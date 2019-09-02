#include "symbolic.hpp"

#include "symengine/eval.h"
#include "symengine/parser.h"
#include "symengine/subs.h"
#include "symengine/symengine_exception.h"
#include "symengine/visitor.h"

#include "logger.hpp"

#include <sstream>

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

Symbolic::Symbolic(const std::string &expression,
                   const std::vector<std::string> &variables,
                   const std::map<std::string, double> &constants) {
  SPDLOG_DEBUG("parsing {}", expression);
  for (const auto &v : variables) {
    symbols[v] = SymEngine::symbol(v);
  }
  SymEngine::map_basic_basic d;
  for (const auto &p : constants) {
    d[SymEngine::symbol(p.first)] = SymEngine::real_double(p.second);
    SPDLOG_DEBUG("  - constant {} = {}", p.first, p.second);
  }
  expr = SymEngine::parse(expression)->subs(d);
  SPDLOG_DEBUG("  --> {}", *expr);
}

std::string Symbolic::simplify() const { return toString(expr); }

std::string Symbolic::diff(const std::string &var) const {
  auto dexpr_dvar = expr->diff(symbols.at(var));
  return toString(dexpr_dvar);
}

std::string Symbolic::toString(
    const SymEngine::RCP<const SymEngine::Basic> &e) const {
  SymEngine::muPrinter p;
  return p.apply(e);
}

}  // namespace symbolic
