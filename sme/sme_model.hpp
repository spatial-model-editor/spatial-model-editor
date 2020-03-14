#pragma once

#include <map>
#include <memory>
#include <string>
#include <vector>

#include "sme_compartment.hpp"

namespace sbml {
class SbmlDocWrapper;
}

namespace pybind11 {
class module;
}

namespace sme {

void pybindModel(pybind11::module& m);

class Model {
 private:
  std::unique_ptr<sbml::SbmlDocWrapper> s;
  std::vector<Compartment> compartments;
  void importSBML(const std::string& filename);

 public:
  explicit Model(const std::string& filename);
  void exportSBML(const std::string& filename);
  void setName(const std::string& name);
  std::string getName() const;
  std::map<std::string, Compartment*> getCompartments();
  std::string getStr() const;
};

}  // namespace sme
