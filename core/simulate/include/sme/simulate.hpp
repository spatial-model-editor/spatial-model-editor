// Simulator

#pragma once

#include "sme/image_stack.hpp"
#include "sme/model_settings.hpp"
#include "sme/simulate_data.hpp"
#include "sme/simulate_options.hpp"
#include <QImage>
#include <QRgb>
#include <QSize>
#include <atomic>
#include <cstddef>
#include <functional>
#include <limits>
#include <map>
#include <memory>
#include <queue>
#include <string>
#include <vector>

namespace sme {

namespace model {
class Model;
}

namespace geometry {
class Compartment;
}

namespace simulate {

class BaseSim;

struct SimEvent {
  double time;
  std::vector<std::string> ids;
};

class Simulation {
private:
  std::unique_ptr<BaseSim> simulator;
  std::vector<const geometry::Compartment *> compartments;
  std::vector<std::string> compartmentIds;
  std::map<std::string, double, std::less<>> eventSubstitutions{};
  // compartment->species
  std::vector<std::vector<std::string>> compartmentSpeciesIds;
  std::vector<std::vector<std::string>> compartmentSpeciesNames;
  std::vector<std::vector<std::size_t>> compartmentSpeciesIndices;
  std::vector<std::vector<QRgb>> compartmentSpeciesColors;
  model::Model &model;
  model::SimulationSettings *settings;
  SimulationData *data;
  common::Volume imageSize;
  std::atomic<bool> isRunning{false};
  std::atomic<bool> stopRequested{false};
  std::atomic<std::size_t> nCompletedTimesteps{0};
  std::queue<SimEvent> simEvents;
  void initModel();
  void initEvents();
  void applyNextEvent();
  void updateConcentrations(double t);

public:
  explicit Simulation(model::Model &smeModel);
  ~Simulation();

  std::size_t doTimesteps(double time, std::size_t nSteps = 1,
                          double timeout_ms = -1.0);
  std::size_t doMultipleTimesteps(
      const std::vector<std::pair<std::size_t, double>> &timesteps,
      double timeout_ms = -1.0,
      const std::function<bool()> &stopRunningCallback = {});
  [[nodiscard]] const std::string &errorMessage() const;
  [[nodiscard]] const common::ImageStack &errorImages() const;
  [[nodiscard]] const std::vector<std::string> &getCompartmentIds() const;
  [[nodiscard]] const std::vector<std::string> &
  getSpeciesIds(std::size_t compartmentIndex) const;
  [[nodiscard]] const std::vector<QRgb> &
  getSpeciesColors(std::size_t compartmentIndex) const;
  [[nodiscard]] const std::vector<double> &getTimePoints() const;
  [[nodiscard]] const AvgMinMax &getAvgMinMax(std::size_t timeIndex,
                                              std::size_t compartmentIndex,
                                              std::size_t speciesIndex) const;
  [[nodiscard]] std::vector<double> getConc(std::size_t timeIndex,
                                            std::size_t compartmentIndex,
                                            std::size_t speciesIndex) const;
  [[nodiscard]] std::vector<double>
  getConcArray(std::size_t timeIndex, std::size_t compartmentIndex,
               std::size_t speciesIndex) const;
  void applyConcsToModel(model::Model &m, std::size_t timeIndex) const;
  [[nodiscard]] std::vector<double> getDcdt(std::size_t compartmentIndex,
                                            std::size_t speciesIndex) const;
  [[nodiscard]] std::vector<double>
  getDcdtArray(std::size_t compartmentIndex, std::size_t speciesIndex) const;
  [[nodiscard]] double getLowerOrderConc(std::size_t compartmentIndex,
                                         std::size_t speciesIndex,
                                         std::size_t pixelIndex) const;
  [[nodiscard]] common::ImageStack
  getConcImage(std::size_t timeIndex,
               const std::vector<std::vector<std::size_t>> &speciesToDraw = {},
               bool normaliseOverAllTimepoints = false,
               bool normaliseOverAllSpecies = false) const;
  [[nodiscard]] const std::vector<std::string> &
  getPyNames(std::size_t compartmentIndex) const;
  [[nodiscard]] std::vector<std::vector<double>>
  getPyConcs(std::size_t timeIndex, std::size_t compartmentIndex) const;
  [[nodiscard]] std::vector<std::vector<double>>
  getPyDcdts(std::size_t compartmentIndex) const;
  [[nodiscard]] std::size_t getNCompletedTimesteps() const;
  [[nodiscard]] const SimulationData &getSimulationData() const;
  [[nodiscard]] bool getIsRunning() const;
  [[nodiscard]] bool getIsStopping() const;
  void requestStop();
};

} // namespace simulate

} // namespace sme
