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

/**
 * @brief Parse, manipulate, and evaluate symbolic expressions.
 *
 * Supports function expansion, constant substitution, symbolic derivatives, and
 * optional LLVM compilation for fast repeated evaluation.
 */
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
  /**
   * @brief Construct an empty symbolic object.
   */
  Symbolic();
  /**
   * @brief Construct and parse multiple expressions.
   * @param expressions Expressions to parse.
   * @param variables Allowed variable symbols.
   * @param constants Constant substitutions.
   * @param functions User-defined functions.
   * @param allow_unknown_symbols If ``true``, skip unknown-symbol checks.
   */
  explicit Symbolic(
      const std::vector<std::string> &expressions,
      const std::vector<std::string> &variables = {},
      const std::vector<std::pair<std::string, double>> &constants = {},
      const std::vector<SymbolicFunction> &functions = {},
      bool allow_unknown_symbols = false);
  /**
   * @brief Construct and parse a single expression.
   * @param expression Expression to parse.
   * @param variables Allowed variable symbols.
   * @param constants Constant substitutions.
   * @param functions User-defined functions.
   * @param allow_unknown_symbols If ``true``, skip unknown-symbol checks.
   */
  explicit Symbolic(
      const std::string &expression,
      const std::vector<std::string> &variables = {},
      const std::vector<std::pair<std::string, double>> &constants = {},
      const std::vector<SymbolicFunction> &functions = {},
      bool allow_unknown_symbols = false);
  /**
   * @brief Parse multiple expressions.
   *
   * @param expressions Expressions to parse.
   * @param variables Allowed variable symbols.
   * @param constants Constant substitutions.
   * @param functions User-defined functions.
   * @param allow_unknown_symbols If ``true``, skip unknown-symbol checks.
   * @returns ``true`` if parsing succeeded.
   */
  bool parse(const std::vector<std::string> &expressions,
             const std::vector<std::string> &variables = {},
             const std::vector<std::pair<std::string, double>> &constants = {},
             const std::vector<SymbolicFunction> &functions = {},
             bool allow_unknown_symbols = false);
  /**
   * @brief Parse a single expression.
   *
   * @param expression Expression to parse.
   * @param variables Allowed variable symbols.
   * @param constants Constant substitutions.
   * @param functions User-defined functions.
   * @param allow_unknown_symbols If ``true``, skip unknown-symbol checks.
   * @returns ``true`` if parsing succeeded.
   */
  bool parse(const std::string &expression,
             const std::vector<std::string> &variables = {},
             const std::vector<std::pair<std::string, double>> &constants = {},
             const std::vector<SymbolicFunction> &functions = {},
             bool allow_unknown_symbols = false);
  /**
   * @brief Compile parsed expressions to LLVM.
   *
   * @param doCSE Enable common subexpression elimination.
   * @param optLevel LLVM optimization level.
   * @returns ``true`` if compilation succeeded.
   */
  bool compile(bool doCSE = true, unsigned optLevel = 3);
  /**
   * @brief Original expression string.
   * @param i Expression index.
   * @returns Original expression string.
   */
  [[nodiscard]] std::string expr(std::size_t i = 0) const;
  /**
   * @brief Expression string after substitutions/inlining.
   * @param i Expression index.
   * @returns Inlined expression string.
   */
  [[nodiscard]] std::string inlinedExpr(std::size_t i = 0) const;
  /**
   * @brief Derivative of expression ``i`` with respect to ``var``.
   * @param var Differentiation variable.
   * @param i Expression index.
   * @returns Derivative expression string.
   */
  [[nodiscard]] std::string diff(const std::string &var,
                                 std::size_t i = 0) const;
  /**
   * @brief Rename variables in parsed/compiled expressions.
   * @param newVariables Replacement variable names.
   */
  void relabel(const std::vector<std::string> &newVariables);
  /**
   * @brief Multiply expression by ``factor`` excluding selected symbols.
   * @param factor Multiplicative scale factor.
   * @param exclusions Symbols to leave unchanged.
   */
  void rescale(double factor, const std::vector<std::string> &exclusions = {});
  /**
   * @brief Evaluate compiled expressions into ``results``.
   * @param results Output results vector.
   * @param vars Input variable values.
   */
  void eval(std::vector<double> &results,
            const std::vector<double> &vars = {}) const;
  /**
   * @brief Evaluate compiled expressions into raw arrays.
   * @param results Output array pointer.
   * @param vars Input variable array pointer.
   */
  void eval(double *results, const double *vars) const;
  /**
   * @brief Returns ``true`` if parsed state is valid.
   * @returns Valid-state flag.
   */
  [[nodiscard]] bool isValid() const;
  /**
   * @brief Returns ``true`` if expressions have been LLVM-compiled.
   * @returns Compiled-state flag.
   */
  [[nodiscard]] bool isCompiled() const;
  /**
   * @brief Latest parse/compile error message.
   * @returns Error message string.
   */
  [[nodiscard]] const std::string &getErrorMessage() const;
  /**
   * @brief Clear all parsed and compiled state.
   */
  void clear();
};

} // namespace sme::common
