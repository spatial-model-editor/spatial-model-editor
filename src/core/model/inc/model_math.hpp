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

namespace model {

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
  double eval(const std::map<const std::string, std::pair<double, bool>> &vars =
                  {}) const;
  bool isValid() const;
  const std::string &getErrorMessage() const;
  ModelMath(ModelMath &&) noexcept;
  ModelMath &operator=(ModelMath &&) noexcept;
  ModelMath &operator=(const ModelMath &) = delete;
  ModelMath(const ModelMath &) = delete;
  ~ModelMath();
};

} // namespace model
