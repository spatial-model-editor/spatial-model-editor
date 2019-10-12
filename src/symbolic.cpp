#include "symbolic.hpp"

#include <sstream>

#include "logger.hpp"
#include "symengine/basic.h"
#include "symengine/eval.h"
#include "symengine/lambda_double.h"
#include "symengine/llvm_double.h"
#include "symengine/parser.h"
#include "symengine/printers/strprinter.h"
#include "symengine/subs.h"
#include "symengine/symbol.h"
#include "symengine/symengine_exception.h"
#include "symengine/symengine_rcp.h"
#include "symengine/visitor.h"

namespace SymEngine {

// modify string printer to use ^ for power operator instead of **
#ifdef __GNUC__
#pragma GCC diagnostic push
// ignore warning that base clase has non-virtual destructor
#pragma GCC diagnostic ignored "-Wnon-virtual-dtor"
#endif
class muPrinter : public StrPrinter {
 protected:
  virtual void _print_pow(std::ostringstream &o, const RCP<const Basic> &a,
                          const RCP<const Basic> &b) override;
};
#ifdef __GNUC__
#pragma GCC diagnostic pop
#endif

}  // namespace SymEngine

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

static std::string toString(const SymEngine::RCP<const SymEngine::Basic> &e) {
  SymEngine::muPrinter p;
  return p.apply(e);
}

class Symbolic::SymEngineImpl {
 public:
  SymEngine::vec_basic expr;
  SymEngine::vec_basic varVec;
  SymEngine::LambdaRealDoubleVisitor lambda;
  SymEngine::LLVMDoubleVisitor lambdaLLVM;
  std::map<std::string, SymEngine::RCP<const SymEngine::Symbol>> symbols;
  void init(const std::vector<std::string> &expressions,
            const std::vector<std::string> &variables,
            const std::map<std::string, double> &constants);
  void relabel(const std::vector<std::string> &newVariables);
};

void Symbolic::SymEngineImpl::init(
    const std::vector<std::string> &expressions,
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
    SPDLOG_DEBUG("  - constant {} = {}", p.first, p.second);
    d[SymEngine::symbol(p.first)] = SymEngine::real_double(p.second);
  }
  for (const auto &expression : expressions) {
    SPDLOG_DEBUG("expr {}", expression);
    expr.push_back(SymEngine::parse(expression)->subs(d));
    SPDLOG_DEBUG("  --> {}", toString(expr.back()));
  }

  // NOTE: don't do symbolic CSE - segfaults!
  lambda.init(varVec, expr, false);
  // compile with LLVM - again no symbolic CSE to avoid segfaults.
  lambdaLLVM.init(varVec, expr, false, 2);
}

void Symbolic::SymEngineImpl::relabel(
    const std::vector<std::string> &newVariables) {
  if (varVec.size() != newVariables.size()) {
    SPDLOG_WARN(
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
    SPDLOG_DEBUG("relabeling {} -> {}", toString(varVec.at(i)),
                 toString(newVarVec.at(i)));
  }
  // substitute new variables into all expressions
  for (auto &e : expr) {
    SPDLOG_DEBUG("expr {}", toString(e));
    e = e->subs(d);
    SPDLOG_DEBUG("  -> {}", toString(e));
  }
  // replace old variables with new variables in vector & map
  std::swap(varVec, newVarVec);
  std::swap(symbols, newSymbols);
}

Symbolic::Symbolic(const std::vector<std::string> &expressions,
                   const std::vector<std::string> &variables,
                   const std::map<std::string, double> &constants)
    : pSymEngineImpl(std::make_shared<SymEngineImpl>()) {
  pSymEngineImpl->init(expressions, variables, constants);
}

std::string Symbolic::simplify(std::size_t i) const {
  return toString(pSymEngineImpl->expr.at(i));
}

std::string Symbolic::diff(const std::string &var, std::size_t i) const {
  auto dexpr_dvar =
      pSymEngineImpl->expr.at(i)->diff(pSymEngineImpl->symbols.at(var));
  return toString(dexpr_dvar);
}

void Symbolic::relabel(const std::vector<std::string> &newVariables) {
  pSymEngineImpl->relabel(newVariables);
}

void Symbolic::eval(std::vector<double> &results,
                    const std::vector<double> &vars) {
  pSymEngineImpl->lambda.call(results.data(), vars.data());
}

void Symbolic::evalLLVM(std::vector<double> &results,
                        const std::vector<double> &vars) {
  pSymEngineImpl->lambdaLLVM.call(results.data(), vars.data());
}

}  // namespace symbolic
