#pragma once

#include "sme/model_species_types.hpp"
#include "sme_common.hpp"
#include <pybind11/pybind11.h>
#include <string>

namespace sme {

namespace model {
class Model;
}

void pybindSpecies(pybind11::module &m);

class Species {
private:
  model::Model *s;
  std::string id;

public:
  Species() = default;
  explicit Species(model::Model *sbmlDocWrapper, const std::string &sId);
  [[nodiscard]] std::string getName() const;
  void setName(const std::string &name);
  [[nodiscard]] double getDiffusionConstant() const;
  void setDiffusionConstant(double diffusionConstant);
  [[nodiscard]] model::ConcentrationType getInitialConcentrationType() const;
  [[nodiscard]] double getUniformInitialConcentration() const;
  void setUniformInitialConcentration(double value);
  [[nodiscard]] std::string getAnalyticInitialConcentration() const;
  void setAnalyticInitialConcentration(const std::string &expression);
  [[nodiscard]] pybind11::array_t<double> getImageInitialConcentration() const;
  void setImageInitialConcentration(pybind11::array_t<double> array);
  [[nodiscard]] std::string getStr() const;
};

} // namespace sme

PYBIND11_MAKE_OPAQUE(std::vector<sme::Species>)
