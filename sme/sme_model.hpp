#pragma once

#include <map>
#include <memory>
#include <string>
#include <vector>

#include "sbml.hpp"
#include "simulate.hpp"
#include "sme_compartment.hpp"

namespace pybind11 {
class module;
}

namespace sme {

void pybindModel(const pybind11::module& m);

class Model {
 private:
  std::unique_ptr<sbml::SbmlDocWrapper> s;
  std::unique_ptr<simulate::Simulation> sim;
  std::vector<Compartment> compartments;
  void importSbmlFile(const std::string& filename);

 public:
  explicit Model(const std::string& filename);
  void exportSbmlFile(const std::string& filename) const;
  void simulate(double simulationTime, double imageInterval);
  std::vector<double> simulationTimePoints() const;
  std::vector<std::vector<std::vector<int>>> concentrationImage(
      std::size_t timePointIndex) const;
  std::vector<std::vector<std::vector<int>>> compartmentImage() const;
  void setName(const std::string& name);
  std::string getName() const;
  std::map<std::string, Compartment*> getCompartments();
  std::string getStr() const;
};

}  // namespace sme
