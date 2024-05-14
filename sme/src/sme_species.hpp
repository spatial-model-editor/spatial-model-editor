#pragma once

#include "sme/model_species_types.hpp"
#include "sme_common.hpp"
#include <nanobind/nanobind.h>
#include <string>
#include <vector>

namespace sme::model {
class Model;
}

namespace pysme {

void bindSpecies(nanobind::module_ &m);

class Species {
private:
  ::sme::model::Model *s;
  std::string id;

public:
  Species() = default;
  explicit Species(::sme::model::Model *sbmlDocWrapper, const std::string &sId);
  [[nodiscard]] std::string getName() const;
  void setName(const std::string &name);
  [[nodiscard]] double getDiffusionConstant() const;
  void setDiffusionConstant(double diffusionConstant);
  [[nodiscard]] ::sme::model::ConcentrationType
  getInitialConcentrationType() const;
  [[nodiscard]] double getUniformInitialConcentration() const;
  void setUniformInitialConcentration(double value);
  [[nodiscard]] std::string getAnalyticInitialConcentration() const;
  void setAnalyticInitialConcentration(const std::string &expression);
  [[nodiscard]] nanobind::ndarray<nanobind::numpy, double>
  getImageInitialConcentration() const;
  void setImageInitialConcentration(
      nanobind::ndarray<nanobind::numpy, double> array);
  [[nodiscard]] std::string getStr() const;
};

} // namespace pysme

NB_MAKE_OPAQUE(std::vector<pysme::Species>)
