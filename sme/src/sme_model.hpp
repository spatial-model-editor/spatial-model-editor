#pragma once

#include "sme/model.hpp"
#include "sme/simulate.hpp"
#include "sme_common.hpp"
#include "sme_compartment.hpp"
#include "sme_membrane.hpp"
#include "sme_parameter.hpp"
#include "sme_simulationresult.hpp"
#include <memory>
#include <nanobind/nanobind.h>
#include <optional>
#include <string>
#include <vector>

namespace pysme {

void bindModel(nanobind::module_ &m);

using SimulationSettings = ::sme::model::SimulationSettings;

class Model {
private:
  std::unique_ptr<::sme::model::Model> s;
  std::unique_ptr<::sme::simulate::Simulation> sim;
  void init();

public:
  explicit Model(const std::string &filename = {});
  void importFile(const std::string &filename);
  void importSbmlString(const std::string &xml);
  [[nodiscard]] std::string getName() const;
  void setName(const std::string &name);
  [[nodiscard]] const SimulationSettings &getSimulationSettings() const;
  SimulationSettings &getSimulationSettings();
  void setSimulationSettings(const SimulationSettings &settings);
  nanobind::ndarray<nanobind::numpy, std::uint8_t> compartment_image() const;
  void importGeometryFromImage(const std::string &filename);
  void exportSbmlFile(const std::string &filename);
  void exportSmeFile(const std::string &filename);
  std::vector<Compartment> compartments;
  std::vector<Membrane> membranes;
  std::vector<Parameter> parameters;
  std::vector<SimulationResult> simulateString(
      std::optional<std::string> lengths, std::optional<std::string> intervals,
      int timeoutSeconds, bool throwOnTimeout,
      std::optional<::sme::simulate::SimulatorType> simulatorType,
      bool continueExistingSimulation, bool returnResults,
      std::optional<int> nThreads,
      const std::optional<SimulationSettings> &simulationSettingsOverride);
  std::vector<SimulationResult> simulateFloat(
      std::optional<double> simulationTime, std::optional<double> imageInterval,
      int timeoutSeconds, bool throwOnTimeout,
      std::optional<::sme::simulate::SimulatorType> simulatorType,
      bool continueExistingSimulation, bool returnResults,
      std::optional<int> nThreads,
      const std::optional<SimulationSettings> &simulationSettingsOverride);
  std::vector<SimulationResult> getSimulationResults();
  [[nodiscard]] std::string getStr() const;
};

} // namespace pysme
