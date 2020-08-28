#include "symbolic.hpp"
#include "logger.hpp"
#include <algorithm>
#include <llvm/Config/llvm-config.h>
#include <locale>
#include <sstream>
#include <symengine/basic.h>
#include <symengine/constants.h>
#include <symengine/dict.h>
#include <symengine/llvm_double.h>
#include <symengine/mul.h>
#include <symengine/number.h>
#include <symengine/parser.h>
#include <symengine/parser/parser.h>
#include <symengine/printers/strprinter.h>
#include <symengine/rational.h>
#include <symengine/real_double.h>
#include <symengine/symbol.h>
#include <symengine/symengine_exception.h>
#include <symengine/symengine_rcp.h>
#include <symengine/visitor.h>

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

} // namespace SymEngine

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
} // namespace SymEngine

namespace symbolic {

static std::string toString(const SymEngine::RCP<const SymEngine::Basic> &e) {
  return SymEngine::muPrinter().apply(e);
}

struct Symbolic::SymEngineImpl {
  SymEngine::vec_basic expr{};
  SymEngine::vec_basic varVec{};
  SymEngine::LLVMDoubleVisitor lambdaLLVM{};
  std::map<std::string, SymEngine::RCP<const SymEngine::Symbol>> symbols{};
  bool valid{false};
  std::string errorMessage{};
  void init(const std::vector<std::string> &expressions,
            const std::vector<std::string> &variables,
            const std::vector<std::pair<std::string, double>> &constants);
  void compile(bool doCSE, unsigned optLevel);
  void relabel(const std::vector<std::string> &newVariables);
};

void Symbolic::SymEngineImpl::init(
    const std::vector<std::string> &expressions,
    const std::vector<std::string> &variables,
    const std::vector<std::pair<std::string, double>> &constants) {
  SPDLOG_DEBUG("parsing {} expressions", expressions.size());
  valid = true;
  for (const auto &v : variables) {
    SPDLOG_DEBUG("  - variable {}", v);
    symbols[v] = SymEngine::symbol(v);
    varVec.push_back(symbols.at(v));
  }
  SymEngine::map_basic_basic d;
  for (const auto &[name, value] : constants) {
    SPDLOG_DEBUG("  - constant {} = {}", name, value);
    d[SymEngine::symbol(name)] = SymEngine::real_double(value);
  }
  // hack until https://github.com/symengine/symengine/issues/1566 is resolved:
  // (SymEngine parser relies on strtod and assumes C locale)
  std::locale userLocale = std::locale::global(std::locale::classic());
  SymEngine::Parser parser;
  for (const auto &expression : expressions) {
    SPDLOG_DEBUG("expr {}", expression);
    // parse expression & substitute all supplied numeric constants
    try {
      expr.push_back(parser.parse(expression)->subs(d));
    } catch (const SymEngine::SymEngineException &e) {
      // if SymEngine failed to parse, capture error message
      SPDLOG_WARN("{}", e.what());
      valid = false;
      errorMessage = e.what();
      std::locale::global(userLocale);
      return;
    }
    SPDLOG_DEBUG("  --> {}", toString(expr.back()));
    // check that all remaining symbols are in the variables vector
    auto fs = SymEngine::free_symbols(*expr.back());
    if (auto iter = find_if(fs.cbegin(), fs.cend(),
                            [&v = variables](const auto &s) {
                              return std::find(v.cbegin(), v.cend(),
                                               toString(s)) == v.cend();
                            });
        iter != fs.cend()) {
      valid = false;
      errorMessage = "Unknown symbol: " + toString(*iter);
      std::locale::global(userLocale);
      return;
    }
  }
  std::locale::global(userLocale);
}

void Symbolic::SymEngineImpl::compile(bool doCSE, unsigned optLevel) {
  SPDLOG_DEBUG("compiling expression");
  lambdaLLVM.init(varVec, expr, doCSE, optLevel);
}

void Symbolic::SymEngineImpl::relabel(
    const std::vector<std::string> &newVariables) {
  if (varVec.size() != newVariables.size()) {
    SPDLOG_WARN("cannot relabel variables: newVariables size {} "
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

std::string divide(const std::string &expr, const std::string &var) {
  std::locale userLocale = std::locale::global(std::locale::classic());
  std::string result = SymEngine::muPrinter().apply(
      SymEngine::div(SymEngine::parse(expr), SymEngine::symbol(var)));
  std::locale::global(userLocale);
  return result;
}

const char *getLLVMVersion() { return LLVM_VERSION_STRING; }

Symbolic::Symbolic() = default;

Symbolic::Symbolic(const std::vector<std::string> &expressions,
                   const std::vector<std::string> &variables,
                   const std::vector<std::pair<std::string, double>> &constants,
                   bool compile, bool doCSE, unsigned optLevel)
    : pSymEngineImpl{std::make_unique<SymEngineImpl>()} {
  pSymEngineImpl->init(expressions, variables, constants);
  if (compile && pSymEngineImpl->valid) {
    pSymEngineImpl->compile(doCSE, optLevel);
  }
}

Symbolic::~Symbolic() = default;

Symbolic::Symbolic(Symbolic &&) noexcept = default;

Symbolic &Symbolic::operator=(Symbolic &&) noexcept = default;

void Symbolic::compile(bool doCSE, unsigned optLevel) {
  pSymEngineImpl->compile(doCSE, optLevel);
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
                    const std::vector<double> &vars) const {
  pSymEngineImpl->lambdaLLVM.call(results.data(), vars.data());
}

void Symbolic::eval(double *results, const double *vars) const {
  pSymEngineImpl->lambdaLLVM.call(results, vars);
}

bool Symbolic::isValid() const { return pSymEngineImpl->valid; }

const std::string &Symbolic::getErrorMessage() const {
  return pSymEngineImpl->errorMessage;
}

} // namespace symbolic
