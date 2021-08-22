#include "symbolic.hpp"
#include "logger.hpp"
#include <algorithm>
#include <llvm/Config/llvm-config.h>
#include <locale>
#include <sstream>
#include <symengine/basic.h>
#include <symengine/constants.h>
#include <symengine/dict.h>
#include <symengine/functions.h>
#include <symengine/llvm_double.h>
#include <symengine/mul.h>
#include <symengine/number.h>
#include <symengine/parser.h>
#include <symengine/parser/sbml/sbml_parser.h>
#include <symengine/printers/strprinter.h>
#include <symengine/rational.h>
#include <symengine/real_double.h>
#include <symengine/symbol.h>
#include <symengine/symengine_exception.h>
#include <symengine/symengine_rcp.h>
#include <symengine/visitor.h>

namespace sme::common {

static std::string toString(const SymEngine::RCP<const SymEngine::Basic> &e) {
  return SymEngine::sbml(*e);
}

struct Symbolic::SymEngineFunc {
  std::string name;
  SymEngine::vec_basic args;
  SymEngine::RCP<const SymEngine::Basic> body;
};

struct Symbolic::SymEngineImpl {
  SymEngine::vec_basic exprInlined{};
  SymEngine::vec_basic exprOriginal{};
  SymEngine::vec_basic varVec{};
  SymEngine::LLVMDoubleVisitor lambdaLLVM{};
  std::map<std::string, SymEngine::RCP<const SymEngine::Symbol>> symbols{};
  bool valid{false};
  bool compiled{false};
  std::string errorMessage{};
  void init(const std::vector<std::string> &expressions,
            const std::vector<std::string> &variables,
            const std::vector<std::pair<std::string, double>> &constants,
            const std::vector<Function> &functions);
  void compile(bool doCSE, unsigned optLevel);
  void relabel(const std::vector<std::string> &newVariables);
  void rescale(double factor, const std::vector<std::string> &exclusions = {});
};

void Symbolic::SymEngineImpl::init(
    const std::vector<std::string> &expressions,
    const std::vector<std::string> &variables,
    const std::vector<std::pair<std::string, double>> &constants,
    const std::vector<Function> &functions) {
  SPDLOG_DEBUG("parsing {} expressions", expressions.size());
  valid = true;
  compiled = false;
  for (const auto &v : variables) {
    SPDLOG_DEBUG("  - variable {}", v);
    symbols[v] = SymEngine::symbol(v);
    varVec.push_back(symbols[v]);
  }
  SymEngine::map_basic_basic d;
  for (const auto &[name, value] : constants) {
    SPDLOG_DEBUG("  - constant {} = {}", name, value);
    d[SymEngine::symbol(name)] = SymEngine::real_double(value);
  }
  // hack until https://github.com/symengine/symengine/issues/1566 is resolved:
  // (SymEngine parser relies on strtod and assumes C locale)
  std::locale userLocale = std::locale::global(std::locale::classic());
  SymEngine::SbmlParser parser;
  // map from function id to symengine expressions
  std::map<std::string, Symbolic::SymEngineFunc> symEngineFuncs;
  for (const auto &function : functions) {
    SPDLOG_DEBUG("  - function '{}: {}'", function.id, function.body);
    auto &symEngineFunc = symEngineFuncs[function.id];
    symEngineFunc.name = function.name;
    for (const auto &arg : function.args) {
      symEngineFunc.args.push_back(parser.parse(arg));
    }
    symEngineFunc.body = parser.parse(function.body);
  }
  for (const auto &expression : expressions) {
    SymEngine::map_basic_basic fns;
    SPDLOG_DEBUG("expr {}", expression);
    // parse expression & substitute all supplied functions & numeric constants
    // todo: clean this up - see
    // https://github.com/symengine/symengine/issues/1689
    try {
      int remainingAllowedReplaceLoops{1024};
      auto e = parser.parse(expression);
      SymEngine::RCP<const SymEngine::Basic> ePrevious{SymEngine::zero};
      exprOriginal.push_back(e);
      auto remainingSymbols = SymEngine::function_symbols(*e);
      while (!remainingSymbols.empty() && !eq(*e, *ePrevious) &&
             --remainingAllowedReplaceLoops > 0) {
        ePrevious = e;
        for (const auto &funcSymbol : remainingSymbols) {
          auto name =
              SymEngine::rcp_dynamic_cast<const SymEngine::FunctionSymbol>(
                  funcSymbol)
                  ->get_name();
          if (auto iter = symEngineFuncs.find(name);
              iter != symEngineFuncs.cend()) {
            const auto &f = iter->second;
            const auto &args = funcSymbol->get_args();
            if (args.size() != f.args.size()) {
              valid = false;
              errorMessage =
                  fmt::format("Function '{}' requires {} argument(s), found {}",
                              f.name, f.args.size(), args.size());
              SPDLOG_WARN("{}", errorMessage);
              std::locale::global(userLocale);
              return;
            }
            SymEngine::map_basic_basic arg_map;
            for (std::size_t i = 0; i < args.size(); ++i) {
              arg_map[f.args[i]] = args[i];
            }
            fns[funcSymbol] = f.body->xreplace(arg_map);
          }
        }
        auto eReplaced = e->xreplace(fns);
        while (!eq(*e, *eReplaced) && --remainingAllowedReplaceLoops > 0) {
          e = eReplaced;
          eReplaced = e->xreplace(fns);
        }
        e = eReplaced;
        remainingSymbols = SymEngine::function_symbols(*e);
      }
      if (remainingAllowedReplaceLoops <= 0) {
        valid = false;
        errorMessage = "Recursive function calls not supported";
        SPDLOG_WARN("{}", errorMessage);
        std::locale::global(userLocale);
        return;
      }
      exprInlined.push_back(e->subs(d));
    } catch (const SymEngine::SymEngineException &e) {
      // if SymEngine failed to parse, capture error message
      SPDLOG_WARN("{}", e.what());
      valid = false;
      errorMessage = e.what();
      std::locale::global(userLocale);
      return;
    }
    SPDLOG_DEBUG("  --> {}", toString(exprInlined.back()));
    // check that all remaining symbols are in the variables vector
    auto fs = SymEngine::free_symbols(*exprInlined.back());
    if (auto iter = find_if(fs.cbegin(), fs.cend(),
                            [&v = variables](const auto &s) {
                              return std::find(v.cbegin(), v.cend(),
                                               toString(s)) == v.cend();
                            });
        iter != fs.cend()) {
      valid = false;
      errorMessage = "Unknown symbol: " + toString(*iter);
      SPDLOG_WARN("{}", errorMessage);
      std::locale::global(userLocale);
      return;
    }
    auto fn = SymEngine::function_symbols(*exprInlined.back());
    if (!fn.empty()) {
      valid = false;
      errorMessage = "Unknown function: " + toString(*fn.begin());
      SPDLOG_WARN("{}", errorMessage);
      std::locale::global(userLocale);
      return;
    }
  }
  std::locale::global(userLocale);
}

void Symbolic::SymEngineImpl::compile(bool doCSE, unsigned optLevel) {
  SPDLOG_DEBUG("compiling expression:");
#if SPDLOG_ACTIVE_LEVEL <= SPDLOG_LEVEL_TRACE
  if (varVec.size() == exprInlined.size()) {
    for (std::size_t i = 0; i < varVec.size(); ++i) {
      SPDLOG_TRACE("  [{}] <- {}", toString(varVec[i]),
                   toString(exprInlined[i]));
    }
  }
#endif
  try {
    lambdaLLVM.init(varVec, exprInlined, doCSE, optLevel);
  } catch (const std::exception &e) {
    // if SymEngine failed to compile, capture error message
    SPDLOG_WARN("{}", e.what());
    valid = false;
    compiled = false;
    errorMessage = "Failed to compile expression: ";
    errorMessage.append(e.what());
    return;
  }
  compiled = true;
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
    const auto &v = newVariables[i];
    newSymbols[v] = SymEngine::symbol(v);
    newVarVec.push_back(newSymbols[v]);
    d[varVec[i]] = newVarVec[i];
    SPDLOG_DEBUG("relabeling {} -> {}", toString(varVec[i]),
                 toString(newVarVec[i]));
  }
  // substitute new variables into all expressions
  for (auto &e : exprInlined) {
    SPDLOG_DEBUG("exprInlined '{}'", toString(e));
    e = e->subs(d);
    SPDLOG_DEBUG("  -> '{}'", toString(e));
  }
  for (auto &e : exprOriginal) {
    SPDLOG_DEBUG("exprOriginal '{}'", toString(e));
    e = e->subs(d);
    SPDLOG_DEBUG("  -> '{}'", toString(e));
  }
  // replace old variables with new variables in vector & map
  std::swap(varVec, newVarVec);
  std::swap(symbols, newSymbols);
  if (compiled) {
    compile(true, 3);
  }
}

void Symbolic::SymEngineImpl::rescale(
    double factor, const std::vector<std::string> &exclusions) {
  SymEngine::map_basic_basic d;
  auto f = SymEngine::number(factor);
  for (const auto &v : varVec) {
    if (std::find(exclusions.cbegin(), exclusions.cend(), toString(v)) !=
        exclusions.cend()) {
      d[v] = v;
    } else {
      d[v] = SymEngine::mul(v, f);
    }
  }
  for (auto &e : exprInlined) {
    SPDLOG_DEBUG("exprInlined '{}'", toString(e));
    e = e->subs(d);
    SPDLOG_DEBUG("  -> '{}'", toString(e));
  }
  for (auto &e : exprOriginal) {
    SPDLOG_DEBUG("exprOriginal '{}'", toString(e));
    e = e->subs(d);
    SPDLOG_DEBUG("  -> '{}'", toString(e));
  }
  if (compiled) {
    compile(true, 3);
  }
}

std::string symbolicDivide(const std::string &expr, const std::string &var) {
  std::locale userLocale = std::locale::global(std::locale::classic());
  std::string result = toString(
      SymEngine::div(SymEngine::parse_sbml(expr), SymEngine::symbol(var)));
  std::locale::global(userLocale);
  return result;
}

std::string symbolicMultiply(const std::string &expr, const std::string &var) {
  std::locale userLocale = std::locale::global(std::locale::classic());
  std::string result = toString(
      SymEngine::mul(SymEngine::parse_sbml(expr), SymEngine::symbol(var)));
  std::locale::global(userLocale);
  return result;
}

bool symbolicContains(const std::string &expr, const std::string &var) {
  std::locale userLocale = std::locale::global(std::locale::classic());
  auto e{SymEngine::parse_sbml(expr)};
  auto v{SymEngine::symbol(var)};
  auto fs = SymEngine::free_symbols(*e);
  std::locale::global(userLocale);
  return fs.find(v) != fs.cend();
}

const char *getLLVMVersion() { return LLVM_VERSION_STRING; }

Symbolic::Symbolic() = default;

Symbolic::Symbolic(const std::vector<std::string> &expressions,
                   const std::vector<std::string> &variables,
                   const std::vector<std::pair<std::string, double>> &constants,
                   const std::vector<Function> &functions, bool compile,
                   bool doCSE, unsigned optLevel)
    : pSymEngineImpl{std::make_unique<SymEngineImpl>()} {
  pSymEngineImpl->init(expressions, variables, constants, functions);
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

std::string Symbolic::expr(std::size_t i) const {
  return toString(pSymEngineImpl->exprOriginal[i]);
}

std::string Symbolic::inlinedExpr(std::size_t i) const {
  return toString(pSymEngineImpl->exprInlined[i]);
}

std::string Symbolic::diff(const std::string &var, std::size_t i) const {
  auto dexpr_dvar =
      pSymEngineImpl->exprInlined[i]->diff(pSymEngineImpl->symbols[var]);
  return toString(dexpr_dvar);
}

void Symbolic::relabel(const std::vector<std::string> &newVariables) {
  pSymEngineImpl->relabel(newVariables);
}

void Symbolic::rescale(double factor,
                       const std::vector<std::string> &exclusions) {
  pSymEngineImpl->rescale(factor, exclusions);
}

void Symbolic::eval(std::vector<double> &results,
                    const std::vector<double> &vars) const {
  pSymEngineImpl->lambdaLLVM.call(results.data(), vars.data());
}

void Symbolic::eval(double *results, const double *vars) const {
  pSymEngineImpl->lambdaLLVM.call(results, vars);
}

bool Symbolic::isValid() const { return pSymEngineImpl->valid; }

bool Symbolic::isCompiled() const { return pSymEngineImpl->compiled; }

const std::string &Symbolic::getErrorMessage() const {
  return pSymEngineImpl->errorMessage;
}

} // namespace sme::common
