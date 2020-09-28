#pragma once

#include <string>

namespace model {
class Model;
}

namespace pybind11 {
class module;
}

namespace sme {

void pybindSpecies(const pybind11::module &m);

class Species {
private:
  model::Model *s;
  std::string id;

public:
  explicit Species(model::Model *sbmlDocWrapper, const std::string &sId);
  std::string getName() const;
  void setName(const std::string &name);
  double getDiffusionConstant() const;
  void setDiffusionConstant(double diffusionConstant);
  std::string getStr() const;
};

} // namespace sme
