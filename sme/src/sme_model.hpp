#pragma once

#include "model.hpp"
#include "simulate.hpp"
#include "sme_common.hpp"
#include "sme_compartment.hpp"
#include "sme_membrane.hpp"
#include "sme_parameter.hpp"
#include "sme_simulationresult.hpp"
#include <memory>
#include <pybind11/pybind11.h>
#include <string>
#include <vector>

namespace sme {

void pybindModel(pybind11::module &m);

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
  std::vector<Membrane> membranes;
  std::vector<Parameter> parameters;
  PyImageRgb compartmentImage;
  std::vector<SimulationResult> simulate(double simulationTime,
                                         double imageInterval,
                                         int timeoutSeconds = 86400,
                                         bool throwOnTimeout = true);
  std::string getStr() const;
};

} // namespace sme
