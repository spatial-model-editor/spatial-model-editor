#pragma once

#include <map>
#include <memory>
#include <string>
#include <vector>

#include "model.hpp"
#include "simulate.hpp"
#include "sme_compartment.hpp"
#include "sme_parameter.hpp"

namespace pybind11 {
class module;
}

namespace sme {

void pybindModel(const pybind11::module &m);

class Model {
private:
  std::unique_ptr<model::Model> s;
  std::unique_ptr<simulate::Simulation> sim;
  std::vector<Compartment> compartments;
  std::vector<Parameter> parameters;
  void importSbmlFile(const std::string &filename);

public:
  explicit Model(const std::string &filename);
  void exportSbmlFile(const std::string &filename);
  void simulate(double simulationTime, double imageInterval,
                std::size_t maxThreads = 0);
  std::vector<double> simulationTimePoints() const;
  std::vector<std::vector<std::vector<int>>>
  concentrationImage(std::size_t timePointIndex) const;
  std::vector<std::vector<std::vector<int>>> compartmentImage() const;
  void setName(const std::string &name);
  std::string getName() const;
  std::map<std::string, Compartment *> getCompartments();
  std::map<std::string, Parameter *> getParameters();
  std::string getStr() const;
};

} // namespace sme
