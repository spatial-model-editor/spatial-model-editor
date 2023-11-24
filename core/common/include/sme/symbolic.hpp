// Symbolic
//  - takes a vector of math expressions as strings with
//     - variables (to remain variables after compilation)
//     - constants and their numeric values (to be inlined during compilation)
//     - functions with args & definition (to be inlined during compilation)
//  - parses & validates expressions
//  - returns simplified expressions with constants/functions inlined as string
//  - returns differential of any expression wrt any variable as string
//  - compiles expressions using LLVM for fast repeated evaluation

#pragma once

#include "sme/symbolic_function.hpp"
#include <cstddef>
#include <memory>
#include <string>
#include <symengine/basic.h>
#include <symengine/llvm_double.h>
#include <symengine/parser/sbml/sbml_parser.h>
#include <symengine/symengine_rcp.h>
#include <utility>
#include <vector>

namespace sme::common {

class Symbolic {
private:
  std::unique_ptr<SymEngine::LLVMDoubleVisitor> lambdaLLVM{};
  SymEngine::vec_basic exprInlined{};
  SymEngine::vec_basic exprOriginal{};
  SymEngine::vec_basic varVec{};
  std::map<std::string, SymEngine::RCP<const SymEngine::Symbol>, std::less<>>
      symbols{};
  std::string errorMessage{};
  SymEngine::SbmlParser parser{};
  bool valid{false};
  bool compiled{false};

public:
  Symbolic();
  explicit Symbolic(
      const std::vector<std::string> &expressions,
      const std::vector<std::string> &variables = {},
      const std::vector<std::pair<std::string, double>> &constants = {},
      const std::vector<SymbolicFunction> &functions = {},
      bool allow_unknown_symbols = false);
  explicit Symbolic(
      const std::string &expression,
      const std::vector<std::string> &variables = {},
      const std::vector<std::pair<std::string, double>> &constants = {},
      const std::vector<SymbolicFunction> &functions = {},
      bool allow_unknown_symbols = false);
  bool parse(const std::vector<std::string> &expressions,
             const std::vector<std::string> &variables = {},
             const std::vector<std::pair<std::string, double>> &constants = {},
             const std::vector<SymbolicFunction> &functions = {},
             bool allow_unknown_symbols = false);
  bool parse(const std::string &expression,
             const std::vector<std::string> &variables = {},
             const std::vector<std::pair<std::string, double>> &constants = {},
             const std::vector<SymbolicFunction> &functions = {},
             bool allow_unknown_symbols = false);
  bool compile(bool doCSE = true, unsigned optLevel = 3);
  [[nodiscard]] std::string expr(std::size_t i = 0) const;
  [[nodiscard]] std::string inlinedExpr(std::size_t i = 0) const;
  [[nodiscard]] std::string diff(const std::string &var,
                                 std::size_t i = 0) const;
  void relabel(const std::vector<std::string> &newVariables);
  void rescale(double factor, const std::vector<std::string> &exclusions = {});
  void eval(std::vector<double> &results,
            const std::vector<double> &vars = {}) const;
  void eval(double *results, const double *vars) const;
  [[nodiscard]] bool isValid() const;
  [[nodiscard]] bool isCompiled() const;
  [[nodiscard]] const std::string &getErrorMessage() const;
  void clear();
};

} // namespace sme::common
