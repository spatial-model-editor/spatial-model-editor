// Symbolic algebra routines (simple wrapper around SymEngine)
//  - takes a vector of math expressions as strings
//     - with list of variables
//     - with list of constants and their numeric values
//  - parses expressions
//  - returns simplified expressions with constants inlined as string
//  - returns differential of any expression wrt any variable as string
//  - evaluates expressions (with LLVM compilation)

#pragma once

#include <cstddef>
#include <memory>
#include <string>
#include <utility>
#include <vector>

namespace sme::common {

// divide given expression by a variable
std::string symbolicDivide(const std::string &expr, const std::string &var);

// multiply given expression by a variable
std::string symbolicMultiply(const std::string &expr, const std::string &var);

// check if an expression contains a variable
bool symbolicContains(const std::string &expr, const std::string &var);

const char *getLLVMVersion();

struct Function {
  std::string id;
  std::string name;
  std::vector<std::string> args;
  std::string body;
};

class Symbolic {
private:
  struct SymEngineImpl;
  struct SymEngineFunc;
  std::unique_ptr<SymEngineImpl> pSymEngineImpl;

public:
  Symbolic();
  explicit Symbolic(
      const std::vector<std::string> &expressions,
      const std::vector<std::string> &variables = {},
      const std::vector<std::pair<std::string, double>> &constants = {},
      const std::vector<Function> &functions = {}, bool compile = true,
      bool doCSE = true, unsigned optLevel = 3);
  explicit Symbolic(
      const std::string &expression,
      const std::vector<std::string> &variables = {},
      const std::vector<std::pair<std::string, double>> &constants = {},
      const std::vector<Function> &functions = {}, bool compile = true,
      bool doCSE = true, unsigned optLevel = 3)
      : Symbolic(std::vector<std::string>{expression}, variables, constants,
                 functions, compile, doCSE, optLevel) {}
  Symbolic(Symbolic &&) noexcept;
  Symbolic(const Symbolic &) = delete;
  Symbolic &operator=(Symbolic &&) noexcept;
  Symbolic &operator=(const Symbolic &) = delete;
  ~Symbolic();

  void compile(bool doCSE = true, unsigned optLevel = 3);
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
};

} // namespace sme::common
