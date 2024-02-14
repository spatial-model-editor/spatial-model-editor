#include "sme/symbolic.hpp"
#include "sme/logger.hpp"
#include <map>
#include <ranges>

namespace sme::common {

struct SymEngineFunc {
  std::string name{};
  SymEngine::vec_basic args{};
  SymEngine::RCP<const SymEngine::Basic> body{};
  SymEngineFunc() = default;
  explicit SymEngineFunc(const SymbolicFunction &symbolicFunction)
      : name{symbolicFunction.name} {
    SymEngine::SbmlParser parser;
    for (const auto &arg : symbolicFunction.args) {
      args.push_back(parser.parse(arg));
    }
    body = parser.parse(symbolicFunction.body);
  }
};

Symbolic::Symbolic() = default;

Symbolic::Symbolic(const std::vector<std::string> &expressions,
                   const std::vector<std::string> &variables,
                   const std::vector<std::pair<std::string, double>> &constants,
                   const std::vector<SymbolicFunction> &functions,
                   bool allow_unknown_symbols) {
  parse(expressions, variables, constants, functions, allow_unknown_symbols);
}

Symbolic::Symbolic(const std::string &expression,
                   const std::vector<std::string> &variables,
                   const std::vector<std::pair<std::string, double>> &constants,
                   const std::vector<SymbolicFunction> &functions,
                   bool allow_unknown_symbols) {
  parse(expression, variables, constants, functions, allow_unknown_symbols);
}

bool Symbolic::parse(
    const std::vector<std::string> &expressions,
    const std::vector<std::string> &variables,
    const std::vector<std::pair<std::string, double>> &constants,
    const std::vector<SymbolicFunction> &functions,
    bool allow_unknown_symbols) {
  clear();
  SPDLOG_DEBUG("parsing {} expressions", expressions.size());
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
  std::locale userLocale{std::locale::global(std::locale::classic())};
  // map from function id to symengine expressions
  std::map<std::string, SymEngineFunc, std::less<>> symEngineFuncs;
  for (const auto &function : functions) {
    SPDLOG_DEBUG("  - function '{}: {}'", function.id, function.body);
    symEngineFuncs[function.id] = SymEngineFunc(function);
  }
  for (const auto &expression : expressions) {
    SymEngine::map_basic_basic fns;
    SPDLOG_DEBUG("expr {}", expression);
    // parse expression & substitute all supplied functions & numeric constants
    // todo: clean this up - see
    // https://github.com/symengine/symengine/issues/1689
    try {
      int remainingAllowedReplaceLoops{1024};
      auto e{parser.parse(expression)};
      SymEngine::RCP<const SymEngine::Basic> ePrevious{SymEngine::zero};
      exprOriginal.push_back(e);
      auto remainingSymbols{function_symbols(*e)};
      while (!remainingSymbols.empty() && !eq(*e, *ePrevious) &&
             --remainingAllowedReplaceLoops > 0) {
        ePrevious = e;
        for (const auto &funcSymbol : remainingSymbols) {
          auto name =
              rcp_dynamic_cast<const SymEngine::FunctionSymbol>(funcSymbol)
                  ->get_name();
          if (auto iter{symEngineFuncs.find(name)};
              iter != symEngineFuncs.cend()) {
            const auto &f = iter->second;
            const auto &args = funcSymbol->get_args();
            if (args.size() != f.args.size()) {
              errorMessage =
                  fmt::format("Error parsing expression '{}': Function '{}' "
                              "requires {} argument(s), found {}",
                              expression, f.name, f.args.size(), args.size());
              SPDLOG_WARN("{}", errorMessage);
              std::locale::global(userLocale);
              return false;
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
        remainingSymbols = function_symbols(*e);
      }
      if (remainingAllowedReplaceLoops <= 0) {
        errorMessage = fmt::format("Error parsing expression '{}': Recursive "
                                   "function calls are not supported",
                                   expression);
        SPDLOG_WARN("{}", errorMessage);
        std::locale::global(userLocale);
        return false;
      }
      exprInlined.push_back(e->subs(d));
    } catch (const SymEngine::SymEngineException &e) {
      // if SymEngine failed to parse, capture error message
      SPDLOG_WARN("{}", e.what());
      errorMessage = fmt::format("Error parsing expression '{}': {}",
                                 expression, e.what());
      std::locale::global(userLocale);
      return false;
    }
    SPDLOG_DEBUG("  --> {}", sbml(*exprInlined.back()));
    if (!allow_unknown_symbols) {
      // check that all remaining symbols are in the variables vector
      auto fs{free_symbols(*exprInlined.back())};
      if (auto iter = std::ranges::find_if(
              fs,
              [&v = variables](const auto &s) {
                return std::ranges::find(v, sbml(*s)) == std::cend(v);
              });
          iter != std::cend(fs)) {
        errorMessage =
            fmt::format("Error parsing expression '{}': Unknown symbol '{}'",
                        expression, sbml(*(*iter)));
        SPDLOG_WARN("{}", errorMessage);
        std::locale::global(userLocale);
        return false;
      }
    }
    auto fn{function_symbols(*exprInlined.back())};
    if (!fn.empty()) {
      errorMessage =
          fmt::format("Error parsing expression '{}': Unknown function '{}'",
                      expression, sbml(*(*fn.begin())));
      SPDLOG_WARN("{}", errorMessage);
      std::locale::global(userLocale);
      return false;
    }
  }
  valid = true;
  std::locale::global(userLocale);
  return true;
}

bool Symbolic::parse(
    const std::string &expression, const std::vector<std::string> &variables,
    const std::vector<std::pair<std::string, double>> &constants,
    const std::vector<SymbolicFunction> &functions,
    bool allow_unknown_symbols) {
  return parse(std::vector<std::string>{expression}, variables, constants,
               functions, allow_unknown_symbols);
}

bool Symbolic::compile(bool doCSE, unsigned optLevel) {
  lambdaLLVM = std::make_unique<SymEngine::LLVMDoubleVisitor>();
  if (!valid) {
    return false;
  }
  SPDLOG_DEBUG("compiling expression:");
#if SPDLOG_ACTIVE_LEVEL <= SPDLOG_LEVEL_TRACE
  if (varVec.size() == exprInlined.size()) {
    for (std::size_t i = 0; i < varVec.size(); ++i) {
      SPDLOG_TRACE("  [{}] <- {}", sbml(*varVec[i]), sbml(*exprInlined[i]));
    }
  }
#endif
  try {
    lambdaLLVM->init(varVec, exprInlined, doCSE, optLevel);
  } catch (const std::exception &e) {
    // if SymEngine failed to compile, capture error message
    SPDLOG_WARN("{}", e.what());
    valid = false;
    compiled = false;
    errorMessage = fmt::format("Error compiling expression: {}", e.what());
    return false;
  }
  compiled = true;
  return true;
}

std::string Symbolic::expr(std::size_t i) const {
  return sbml(*exprOriginal[i]);
}

std::string Symbolic::inlinedExpr(std::size_t i) const {
  return sbml(*exprInlined[i]);
}

std::string Symbolic::diff(const std::string &var, std::size_t i) const {
  return sbml(*exprInlined[i]->diff(symbols.at(var)));
}

void Symbolic::relabel(const std::vector<std::string> &newVariables) {
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
    SPDLOG_DEBUG("relabeling {} -> {}", sbml(*varVec[i]), sbml(*newVarVec[i]));
  }
  // substitute new variables into all expressions
  for (auto &e : exprInlined) {
    SPDLOG_DEBUG("exprInlined '{}'", sbml(*e));
    e = e->subs(d);
    SPDLOG_DEBUG("  -> '{}'", sbml(*e));
  }
  for (auto &e : exprOriginal) {
    SPDLOG_DEBUG("exprOriginal '{}'", sbml(*e));
    e = e->subs(d);
    SPDLOG_DEBUG("  -> '{}'", sbml(*e));
  }
  // replace old variables with new variables in vector & map
  std::swap(varVec, newVarVec);
  std::swap(symbols, newSymbols);
  if (compiled) {
    compile(true, 3);
  }
}

void Symbolic::rescale(double factor,
                       const std::vector<std::string> &exclusions) {
  SymEngine::map_basic_basic d;
  auto f = SymEngine::number(factor);
  for (const auto &v : varVec) {
    if (std::ranges::find(exclusions, sbml(*v)) != std::cend(exclusions)) {
      d[v] = v;
    } else {
      d[v] = mul(v, f);
    }
  }
  for (auto &e : exprInlined) {
    SPDLOG_DEBUG("exprInlined '{}'", sbml(*e));
    e = e->subs(d);
    SPDLOG_DEBUG("  -> '{}'", sbml(*e));
  }
  for (auto &e : exprOriginal) {
    SPDLOG_DEBUG("exprOriginal '{}'", sbml(*e));
    e = e->subs(d);
    SPDLOG_DEBUG("  -> '{}'", sbml(*e));
  }
  if (compiled) {
    compile(true, 3);
  }
}

void Symbolic::eval(std::vector<double> &results,
                    const std::vector<double> &vars) const {
  lambdaLLVM->call(results.data(), vars.data());
}

void Symbolic::eval(double *results, const double *vars) const {
  lambdaLLVM->call(results, vars);
}

bool Symbolic::isValid() const { return valid; }

bool Symbolic::isCompiled() const { return compiled; }

const std::string &Symbolic::getErrorMessage() const { return errorMessage; }

void Symbolic::clear() {
  lambdaLLVM.reset();
  exprInlined.clear();
  exprOriginal.clear();
  varVec.clear();
  symbols.clear();
  errorMessage.clear();
  valid = false;
  compiled = false;
}
} // namespace sme::common
