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

namespace symbolic {

// divide given expression by a variable
std::string divide(const std::string &expr, const std::string &var);

const char *getLLVMVersion();

class Symbolic {
private:
  struct SymEngineImpl;
  std::unique_ptr<SymEngineImpl> pSymEngineImpl;

public:
  Symbolic();
  explicit Symbolic(
      const std::vector<std::string> &expressions,
      const std::vector<std::string> &variables = {},
      const std::vector<std::pair<std::string, double>> &constants = {},
      bool compile = true);
  explicit Symbolic(
      const std::string &expression,
      const std::vector<std::string> &variables = {},
      const std::vector<std::pair<std::string, double>> &constants = {},
      bool compile = true)
      : Symbolic(std::vector<std::string>{expression}, variables, constants,
                 compile) {}
  Symbolic(Symbolic &&) noexcept;
  Symbolic(const Symbolic &) = delete;
  Symbolic &operator=(Symbolic &&) noexcept;
  Symbolic &operator=(const Symbolic &) = delete;
  ~Symbolic();

  // compile expression (done by default in constructor)
  void compile();
  // simplify given expression
  std::string simplify(std::size_t i = 0) const;
  // differentiate given expression wrt a variable
  std::string diff(const std::string &var, std::size_t i = 0) const;
  // relabel variables
  void relabel(const std::vector<std::string> &newVariables);
  // evaluate compiled expressions
  void eval(std::vector<double> &results,
            const std::vector<double> &vars = {}) const;
  void eval(double *results, const double *vars) const;
  bool isValid() const;
  const std::string &getErrorMessage() const;
};

} // namespace symbolic
