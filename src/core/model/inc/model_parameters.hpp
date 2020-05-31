// SBML parameters

#pragma once

#include <QColor>
#include <QStringList>
#include <map>
#include <optional>
#include <string>

namespace libsbml {
class Model;
class Species;
}  // namespace libsbml

namespace model {

class ModelSpecies;

struct IdNameValue {
  std::string id;
  std::string name;
  double value;
};

struct IdNameExpr {
  std::string id;
  std::string name;
  std::string expr;
};

class ModelParameters {
 public:
  libsbml::Model *sbmlModel = nullptr;

 public:
  ModelParameters();
  explicit ModelParameters(libsbml::Model *model);
  std::vector<IdNameValue> getGlobalConstants() const;
  std::vector<IdNameExpr> getNonConstantParameters() const;
};

}  // namespace sbml
