#pragma once

#include <map>
#include <string>
#include <vector>

#include "sme_reaction.hpp"
#include "sme_species.hpp"

namespace model {
class Model;
}

namespace pybind11 {
class module;
}

namespace sme {

void pybindCompartment(const pybind11::module& m);

class Compartment {
 private:
  model::Model* s;
  std::string id;
  std::vector<Species> species;
  std::vector<Reaction> reactions;

 public:
  explicit Compartment(model::Model* sbmlDocWrapper,
                       const std::string& sId);
  const std::string& getId() const;
  void setName(const std::string& name);
  std::string getName() const;
  std::map<std::string, Species*> getSpecies();
  std::map<std::string, Reaction*> getReactions();
  std::string getStr() const;
};

}  // namespace sme
