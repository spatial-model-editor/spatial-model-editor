// SBML math

#pragma once

#include <map>
#include <memory>
#include <string>
#include <utility>

namespace libsbml {
class Model;
class ASTNode;
} // namespace libsbml

namespace sme::model {

/**
 * @brief Parser/evaluator for SBML math expressions.
 */
class ModelMath {
private:
  const libsbml::Model *sbmlModel{nullptr};
  std::unique_ptr<const libsbml::ASTNode> astNode;
  bool valid{false};
  std::string errorMessage{"Empty expression"};

public:
  /**
   * @brief Construct invalid empty math object.
   */
  ModelMath();
  /**
   * @brief Construct math parser bound to an SBML model.
   */
  explicit ModelMath(const libsbml::Model *model);
  /**
   * @brief Parse expression and update validity state.
   */
  void parse(const std::string &expr);
  /**
   * @brief Evaluate parsed expression.
   */
  [[nodiscard]] double
  eval(const std::map<const std::string, std::pair<double, bool>> &vars = {})
      const;
  /**
   * @brief Returns ``true`` if parsed expression is valid.
   */
  [[nodiscard]] bool isValid() const;
  /**
   * @brief Parse/evaluation error message.
   */
  [[nodiscard]] const std::string &getErrorMessage() const;
};

} // namespace sme::model
