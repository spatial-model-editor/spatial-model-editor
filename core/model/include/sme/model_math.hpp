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

class ModelMath {
private:
  const libsbml::Model *sbmlModel{nullptr};
  std::unique_ptr<const libsbml::ASTNode> astNode;
  bool valid{false};
  std::string errorMessage{"Empty expression"};

public:
  ModelMath();
  explicit ModelMath(const libsbml::Model *model);
  void parse(const std::string &expr);
  [[nodiscard]] double
  eval(const std::map<const std::string, std::pair<double, bool>> &vars = {})
      const;
  [[nodiscard]] bool isValid() const;
  [[nodiscard]] const std::string &getErrorMessage() const;
};

} // namespace sme::model
