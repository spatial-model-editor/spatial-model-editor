#include "symbolic.hpp"
#include "logger.hpp"
#include <llvm/Config/llvm-config.h>
#include <map>
#include <symengine/basic.h>
#include <symengine/llvm_double.h>
#include <symengine/parser/sbml/sbml_parser.h>
#include <symengine/symengine_rcp.h>

namespace sme::common {

using namespace SymEngine;

struct Symbolic::SymEngineFunc {
  std::string name{};
  vec_basic args{};
  RCP<const Basic> body{};
  SymEngineFunc() = default;
  explicit SymEngineFunc(const SymbolicFunction &symbolicFunction);
};

Symbolic::SymEngineFunc::SymEngineFunc(const SymbolicFunction &symbolicFunction)
    : name{symbolicFunction.name} {
  SbmlParser parser;
  for (const auto &arg : symbolicFunction.args) {
    args.push_back(parser.parse(arg));
  }
  body = parser.parse(symbolicFunction.body);
}

struct Symbolic::SymEngineWrapper {
  LLVMDoubleVisitor lambdaLLVM{};
  vec_basic exprInlined{};
  vec_basic exprOriginal{};
  vec_basic varVec{};
  std::map<std::string, RCP<const Symbol>, std::less<>> symbols{};
  std::string errorMessage{};
};

Symbolic::Symbolic() = default;

Symbolic::Symbolic(const std::vector<std::string> &expressions,
                   const std::vector<std::string> &variables,
                   const std::vector<std::pair<std::string, double>> &constants,
                   const std::vector<SymbolicFunction> &functions)
    : se{std::make_unique<SymEngineWrapper>()} {
  SPDLOG_DEBUG("parsing {} expressions", expressions.size());
  for (const auto &v : variables) {
    SPDLOG_DEBUG("  - variable {}", v);
    se->symbols[v] = symbol(v);
    se->varVec.push_back(se->symbols[v]);
  }
  map_basic_basic d;
  for (const auto &[name, value] : constants) {
    SPDLOG_DEBUG("  - constant {} = {}", name, value);
    d[symbol(name)] = real_double(value);
  }
  // hack until https://github.com/symengine/symengine/issues/1566 is resolved:
  // (SymEngine parser relies on strtod and assumes C locale)
  std::locale userLocale{std::locale::global(std::locale::classic())};
  SbmlParser parser;
  // map from function id to symengine expressions
  std::map<std::string, SymEngineFunc, std::less<>> symEngineFuncs;
  for (const auto &function : functions) {
    SPDLOG_DEBUG("  - function '{}: {}'", function.id, function.body);
    symEngineFuncs[function.id] = SymEngineFunc(function);
  }
  for (const auto &expression : expressions) {
    map_basic_basic fns;
    SPDLOG_DEBUG("expr {}", expression);
    // parse expression & substitute all supplied functions & numeric constants
    // todo: clean this up - see
    // https://github.com/symengine/symengine/issues/1689
    try {
      int remainingAllowedReplaceLoops{1024};
      auto e{parser.parse(expression)};
      RCP<const Basic> ePrevious{zero};
      se->exprOriginal.push_back(e);
      auto remainingSymbols{function_symbols(*e)};
      while (!remainingSymbols.empty() && !eq(*e, *ePrevious) &&
             --remainingAllowedReplaceLoops > 0) {
        ePrevious = e;
        for (const auto &funcSymbol : remainingSymbols) {
          auto name =
              rcp_dynamic_cast<const FunctionSymbol>(funcSymbol)->get_name();
          if (auto iter{symEngineFuncs.find(name)};
              iter != symEngineFuncs.cend()) {
            const auto &f = iter->second;
            const auto &args = funcSymbol->get_args();
            if (args.size() != f.args.size()) {
              se->errorMessage =
                  fmt::format("Function '{}' requires {} argument(s), found {}",
                              f.name, f.args.size(), args.size());
              SPDLOG_WARN("{}", se->errorMessage);
              std::locale::global(userLocale);
              return;
            }
            map_basic_basic arg_map;
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
        se->errorMessage = "Recursive function calls not supported";
        SPDLOG_WARN("{}", se->errorMessage);
        std::locale::global(userLocale);
        return;
      }
      se->exprInlined.push_back(e->subs(d));
    } catch (const SymEngineException &e) {
      // if SymEngine failed to parse, capture error message
      SPDLOG_WARN("{}", e.what());
      se->errorMessage = e.what();
      std::locale::global(userLocale);
      return;
    }
    SPDLOG_DEBUG("  --> {}", sbml(*se->exprInlined.back()));
    // check that all remaining symbols are in the variables vector
    auto fs = free_symbols(*se->exprInlined.back());
    if (auto iter = find_if(fs.cbegin(), fs.cend(),
                            [&v = variables](const auto &s) {
                              return std::find(v.cbegin(), v.cend(),
                                               sbml(*s)) == v.cend();
                            });
        iter != fs.cend()) {
      se->errorMessage = "Unknown symbol: " + sbml(*(*iter));
      SPDLOG_WARN("{}", se->errorMessage);
      std::locale::global(userLocale);
      return;
    }
    auto fn{function_symbols(*se->exprInlined.back())};
    if (!fn.empty()) {
      se->errorMessage = "Unknown function: " + sbml(*(*fn.begin()));
      SPDLOG_WARN("{}", se->errorMessage);
      std::locale::global(userLocale);
      return;
    }
  }
  valid = true;
  std::locale::global(userLocale);
}

Symbolic::~Symbolic() = default;

Symbolic::Symbolic(Symbolic &&) noexcept = default;

Symbolic &Symbolic::operator=(Symbolic &&) noexcept = default;

const char *Symbolic::getLLVMVersion() { return LLVM_VERSION_STRING; }

void Symbolic::compile(bool doCSE, unsigned optLevel) {
  if (!valid) {
    return;
  }
  SPDLOG_DEBUG("compiling expression:");
#if SPDLOG_ACTIVE_LEVEL <= SPDLOG_LEVEL_TRACE
  if (se->varVec.size() == se->exprInlined.size()) {
    for (std::size_t i = 0; i < se->varVec.size(); ++i) {
      SPDLOG_TRACE("  [{}] <- {}", sbml(*se->varVec[i]),
                   sbml(*se->exprInlined[i]));
    }
  }
#endif
  try {
    se->lambdaLLVM.init(se->varVec, se->exprInlined, doCSE, optLevel);
  } catch (const std::exception &e) {
    // if SymEngine failed to compile, capture error message
    SPDLOG_WARN("{}", e.what());
    valid = false;
    compiled = false;
    se->errorMessage = "Failed to compile expression: ";
    se->errorMessage.append(e.what());
    return;
  }
  compiled = true;
}

std::string Symbolic::expr(std::size_t i) const {
  return sbml(*se->exprOriginal[i]);
}

std::string Symbolic::inlinedExpr(std::size_t i) const {
  return sbml(*se->exprInlined[i]);
}

std::string Symbolic::diff(const std::string &var, std::size_t i) const {
  return sbml(*se->exprInlined[i]->diff(se->symbols.at(var)));
}

void Symbolic::relabel(const std::vector<std::string> &newVariables) {
  if (se->varVec.size() != newVariables.size()) {
    SPDLOG_WARN("cannot relabel variables: newVariables size {} "
                "does not match number of existing variables {}",
                newVariables.size(), se->varVec.size());
    return;
  }
  decltype(se->varVec) newVarVec;
  decltype(se->symbols) newSymbols;
  map_basic_basic d;
  for (std::size_t i = 0; i < newVariables.size(); ++i) {
    const auto &v = newVariables[i];
    newSymbols[v] = symbol(v);
    newVarVec.push_back(newSymbols[v]);
    d[se->varVec[i]] = newVarVec[i];
    SPDLOG_DEBUG("relabeling {} -> {}", sbml(*se->varVec[i]),
                 sbml(*newVarVec[i]));
  }
  // substitute new variables into all expressions
  for (auto &e : se->exprInlined) {
    SPDLOG_DEBUG("exprInlined '{}'", sbml(*e));
    e = e->subs(d);
    SPDLOG_DEBUG("  -> '{}'", sbml(*e));
  }
  for (auto &e : se->exprOriginal) {
    SPDLOG_DEBUG("exprOriginal '{}'", sbml(*e));
    e = e->subs(d);
    SPDLOG_DEBUG("  -> '{}'", sbml(*e));
  }
  // replace old variables with new variables in vector & map
  std::swap(se->varVec, newVarVec);
  std::swap(se->symbols, newSymbols);
  if (compiled) {
    compile(true, 3);
  }
}

void Symbolic::rescale(double factor,
                       const std::vector<std::string> &exclusions) {
  map_basic_basic d;
  auto f = number(factor);
  for (const auto &v : se->varVec) {
    if (std::find(exclusions.cbegin(), exclusions.cend(), sbml(*v)) !=
        exclusions.cend()) {
      d[v] = v;
    } else {
      d[v] = mul(v, f);
    }
  }
  for (auto &e : se->exprInlined) {
    SPDLOG_DEBUG("exprInlined '{}'", sbml(*e));
    e = e->subs(d);
    SPDLOG_DEBUG("  -> '{}'", sbml(*e));
  }
  for (auto &e : se->exprOriginal) {
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
  se->lambdaLLVM.call(results.data(), vars.data());
}

void Symbolic::eval(double *results, const double *vars) const {
  se->lambdaLLVM.call(results, vars);
}

bool Symbolic::isValid() const { return valid; }

bool Symbolic::isCompiled() const { return compiled; }

const std::string &Symbolic::getErrorMessage() const {
  return se->errorMessage;
}

} // namespace sme::common
