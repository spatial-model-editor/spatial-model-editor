#pragma once
#include "model.hpp"
#include "simulate.hpp"
#include "sme_common.hpp"
#include "sme_compartment.hpp"
#include "sme_membrane.hpp"
#include "sme_parameter.hpp"
#include "sme_simulationresult.hpp"
#include <memory>
#include <string>
#include <vector>

namespace pybind11 {
class module;
}

namespace sme {

void pybindModel(const pybind11::module &m);

class Model {
private:
  std::unique_ptr<model::Model> s;
  std::unique_ptr<simulate::Simulation> sim;
  void importSbmlFile(const std::string &filename);

public:
  explicit Model(const std::string &filename);
  std::string getName() const;
  void setName(const std::string &name);
  void exportSbmlFile(const std::string &filename);
  std::vector<Compartment> compartments;
  const Compartment &getCompartment(const std::string &name) const;
  std::vector<Membrane> membranes;
  const Membrane &getMembrane(const std::string &name) const;
  std::vector<Parameter> parameters;
  const Parameter &getParameter(const std::string &name) const;
  PyImageRgb compartmentImage;
  std::vector<SimulationResult> simulate(double simulationTime,
                                         double imageInterval);
  std::string getStr() const;
};

} // namespace sme
