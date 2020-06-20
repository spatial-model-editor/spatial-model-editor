#pragma once

#include <string>

namespace model {
class Model;
}

namespace pybind11 {
class module;
}

namespace sme {

void pybindSpecies(const pybind11::module& m);

class Species {
 private:
  model::Model* s;
  std::string id;

 public:
  explicit Species(model::Model* sbmlDocWrapper,
                   const std::string& sId);
  const std::string& getId() const;
  void setName(const std::string& name);
  std::string getName() const;
  void setDiffusionConstant(double diffusionConstant);
  double getDiffusionConstant() const;
  std::string getStr() const;
};

}  // namespace sme
