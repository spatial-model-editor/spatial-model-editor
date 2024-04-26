#pragma once

#include "sme_common.hpp"
#include "sme_reaction.hpp"
#include "sme_species.hpp"
#include <nanobind/nanobind.h>
#include <string>
#include <vector>

namespace sme::model {
class Model;
}

namespace pysme {

void bindCompartment(nanobind::module_ &m);

class Compartment {
private:
  ::sme::model::Model *s;
  std::string id;

public:
  Compartment() = default;
  explicit Compartment(::sme::model::Model *sbmlDocWrapper,
                       const std::string &sId);
  [[nodiscard]] std::string getName() const;
  void setName(const std::string &name);
  [[nodiscard]] nanobind::ndarray<nanobind::numpy, bool> geometry_mask() const;
  std::vector<Species> species;
  std::vector<Reaction> reactions;
  [[nodiscard]] std::string getStr() const;
};

} // namespace pysme

NB_MAKE_OPAQUE(std::vector<pysme::Compartment>)
