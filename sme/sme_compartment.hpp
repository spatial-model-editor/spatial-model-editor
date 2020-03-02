#pragma once

#include <map>
#include <string>
#include <vector>

#include "sme_species.hpp"

namespace sbml {
class SbmlDocWrapper;
}

namespace pybind11 {
class module;
}

namespace sme {

void pybindCompartment(pybind11::module& m);

class Compartment {
 private:
  sbml::SbmlDocWrapper* s;
  std::string id;
  std::vector<Species> species;

 public:
  explicit Compartment(sbml::SbmlDocWrapper* sbmlDocWrapper,
                       const std::string& compartmentId);
  const std::string& getId() const;
  void setName(const std::string& name);
  std::string getName() const;
  std::map<std::string, Species*> getSpecies();
  std::string getStr() const;
};

}  // namespace sme
