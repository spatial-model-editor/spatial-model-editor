#pragma once

#include "sme_reaction.hpp"
#include "sme_species.hpp"
#include <map>
#include <string>
#include <vector>

namespace model {
class Model;
}

namespace pybind11 {
class module;
}

namespace sme {

void pybindCompartment(const pybind11::module &m);

class Compartment {
private:
  model::Model *s;
  std::string id;

public:
  explicit Compartment(model::Model *sbmlDocWrapper, const std::string &sId);
  std::string getName() const;
  void setName(const std::string &name);
  std::vector<Species> species;
  const Species &getSpecies(const std::string &name) const;
  std::vector<Reaction> reactions;
  const Reaction &getReaction(const std::string &name) const;
  std::string getStr() const;
};

} // namespace sme
