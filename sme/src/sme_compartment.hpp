#pragma once

#include "sme_common.hpp"
#include "sme_reaction.hpp"
#include "sme_species.hpp"
#include <pybind11/pybind11.h>
#include <string>
#include <vector>

namespace model {
class Model;
}

namespace sme {

void pybindCompartment(pybind11::module &m);

class Compartment {
private:
  model::Model *s;
  std::string id;

public:
  Compartment() = default;
  explicit Compartment(model::Model *sbmlDocWrapper, const std::string &sId);
  [[nodiscard]] std::string getName() const;
  void setName(const std::string &name);
  std::vector<Species> species;
  std::vector<Reaction> reactions;
  [[nodiscard]] PyImageMask getGeometryMask() const;
  [[nodiscard]] std::string getStr() const;
};

} // namespace sme

PYBIND11_MAKE_OPAQUE(std::vector<sme::Compartment>)
