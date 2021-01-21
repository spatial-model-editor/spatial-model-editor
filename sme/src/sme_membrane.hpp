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

void pybindMembrane(pybind11::module &m);

class Membrane {
private:
  model::Model *s;
  std::string id;

public:
  Membrane() = default;
  explicit Membrane(model::Model *sbmlDocWrapper, const std::string &sId);
  std::string getName() const;
  std::vector<Reaction> reactions;
  std::string getStr() const;
};

} // namespace sme

PYBIND11_MAKE_OPAQUE(std::vector<sme::Membrane>)
