#include "sme/symbolic.hpp"
#include "sme/logger.hpp"
#include <map>
#include <ranges>
#include <stack>

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

static SymEngine::RCP<const SymEngine::Basic> substituteFunctions(
    const SymEngine::RCP<const SymEngine::Basic> &expr,
    const std::map<std::string, SymEngineFunc, std::less<>> &funcs,
    SymEngine::map_basic_basic &defs) {
  std::stack<SymEngine::RCP<const SymEngine::Basic>> exprStack;
  SymEngine::set_basic visited;
  SymEngine::map_basic_basic substitutedExprs;
  exprStack.push(expr);

  while (!exprStack.empty()) {
    const auto &subExpr = exprStack.top();
    SPDLOG_TRACE("Processing {}", sbml(*subExpr));
    const auto funcSymbols = function_symbols(*subExpr);
    // push any unvisited children (args or function bodies) onto the stack
    bool expressionCanBeSubstituted = true;
    if (!visited.contains(subExpr)) {
      visited.insert(subExpr);
      for (const auto &funcSymbol : funcSymbols) {
        auto name =
            rcp_dynamic_cast<const SymEngine::FunctionSymbol>(funcSymbol)
                ->get_name();
        if (auto iter{funcs.find(name)}; iter != funcs.cend()) {
          SPDLOG_TRACE("  - expanding function '{}'", name);
          const auto &f = iter->second;
          if (funcSymbol->get_args().size() != f.args.size()) {
            throw SymEngine::SymEngineException(fmt::format(
                "Function '{}' requires {} argument(s), found {}", f.name,
                f.args.size(), funcSymbol->get_args().size()));
          }
          if (!visited.contains(f.body)) {
            exprStack.push(f.body);
            SPDLOG_TRACE("    - push body to stack: {}", sbml(*f.body));
            expressionCanBeSubstituted = false;
          }
          for (const auto &arg : funcSymbol->get_args()) {
            if (!visited.contains(arg)) {
              exprStack.push(arg);
              SPDLOG_TRACE("    - push arg to stack: {}", sbml(*arg));
              expressionCanBeSubstituted = false;
            }
          }
        }
      }
    }
    if (expressionCanBeSubstituted) {
      // all children of this expression have been visited
      // (i.e. all function args and bodies have been substituted)
      // so we can now substitute this expression using substitutedExprs
      SymEngine::map_basic_basic funcMap;
      for (const auto &funcSymbol : funcSymbols) {
        auto name =
            rcp_dynamic_cast<const SymEngine::FunctionSymbol>(funcSymbol)
                ->get_name();
        if (auto iter{funcs.find(name)}; iter != funcs.cend()) {
          SPDLOG_TRACE("  - substituting function {}", name);
          const auto &f = iter->second;
          auto iterBody = substitutedExprs.find(f.body);
          if (iterBody == substitutedExprs.end()) {
            throw SymEngine::SymEngineException(
                fmt::format("Function '{}' calls itself - recursive function "
                            "calls are not supported",
                            name));
          }
          const auto &substitutedBody = iterBody->second;
          SPDLOG_TRACE("    - body: {} -> {}", sbml(*f.body),
                       sbml(*substitutedBody));
          // map function arguments (the arguments in the function definition)
          // to substituted arguments (what was passed to the function in the
          // expression)
          SymEngine::map_basic_basic argMap;
          std::size_t i = 0;
          for (const auto &funcSymbolArg : funcSymbol->get_args()) {
            argMap[f.args[i]] = substitutedExprs.at(funcSymbolArg);
            SPDLOG_TRACE("    - arg: {} -> {}", sbml(*f.args[i]),
                         sbml(*argMap[f.args[i]]));
            ++i;
          }
          funcMap[funcSymbol] = substitutedBody->xreplace(argMap);
          SPDLOG_TRACE("    -> {}", sbml(*funcMap[funcSymbol]));
        }
      }
      substitutedExprs[subExpr] = subExpr->xreplace(funcMap);
      SPDLOG_TRACE("  -> {}", sbml(*substitutedExprs[subExpr]));
      SPDLOG_TRACE("  - removed from stack");
      exprStack.pop();
    }
  }
  // substitute any supplied numeric constants in the final expression
  return substitutedExprs.at(expr)->xreplace(defs);
}

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
    try {
      auto symengineExpr{parser.parse(expression)};
      exprOriginal.push_back(symengineExpr);
      exprInlined.push_back(
          substituteFunctions(symengineExpr, symEngineFuncs, d));
    } catch (const SymEngine::SymEngineException &e) {
      // if SymEngine failed to parse, capture error message
      SPDLOG_WARN("{}", e.what());
      errorMessage = fmt::format("Error parsing expression '{}': {}",
                                 expression, e.what());
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
        return false;
      }
    }
    if (auto fn{function_symbols(*exprInlined.back())}; !fn.empty()) {
      errorMessage =
          fmt::format("Error parsing expression '{}': Unknown function '{}'",
                      expression, sbml(*(*fn.begin())));
      SPDLOG_WARN("{}", errorMessage);
      return false;
    }
  }
  valid = true;
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
