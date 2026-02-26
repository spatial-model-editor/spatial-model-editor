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
#include <shared_mutex>
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

/**
 * @brief Time-point event containing affected variable ids.
 */
struct SimEvent {
  double time;
  std::vector<std::string> ids;
};

/**
 * @brief High-level simulation orchestrator over configured solver backend.
 */
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
  mutable std::shared_mutex dataMutex;
  std::atomic<bool> isRunning{false};
  std::atomic<bool> stopRequested{false};
  std::atomic<std::size_t> nCompletedTimesteps{0};
  std::queue<SimEvent> simEvents;
  void initModel();
  void initEvents();
  void applyNextEvent();
  void updateConcentrations(double t);

public:
  /**
   * @brief Construct simulation for model.
   * @param smeModel Model to simulate.
   */
  explicit Simulation(model::Model &smeModel);
  /**
   * @brief Destructor.
   */
  ~Simulation();

  /**
   * @brief Advance simulation by ``nSteps`` of size ``time``.
   * @param time Timestep size.
   * @param nSteps Number of timesteps.
   * @param timeout_ms Timeout in milliseconds, negative for no timeout.
   * @returns Number of completed timesteps.
   */
  std::size_t doTimesteps(double time, std::size_t nSteps = 1,
                          double timeout_ms = -1.0);
  /**
   * @brief Run multiple simulation segments.
   * @param timesteps Sequence of ``(nSteps, dt)`` segments.
   * @param timeout_ms Timeout in milliseconds, negative for no timeout.
   * @param stopRunningCallback Optional callback for external cancellation.
   * @returns Number of completed timesteps.
   */
  std::size_t doMultipleTimesteps(
      const std::vector<std::pair<std::size_t, double>> &timesteps,
      double timeout_ms = -1.0,
      const std::function<bool()> &stopRunningCallback = {});
  /**
   * @brief Solver error message.
   * @returns Error message string.
   */
  [[nodiscard]] const std::string &errorMessage() const;
  /**
   * @brief Solver error visualization images.
   * @returns Error image stack.
   */
  [[nodiscard]] const common::ImageStack &errorImages() const;
  /**
   * @brief Compartment ids in simulation order.
   * @returns Compartment ids.
   */
  [[nodiscard]] const std::vector<std::string> &getCompartmentIds() const;
  /**
   * @brief Species ids for a compartment.
   * @param compartmentIndex Compartment index.
   * @returns Species ids for compartment.
   */
  [[nodiscard]] const std::vector<std::string> &
  getSpeciesIds(std::size_t compartmentIndex) const;
  /**
   * @brief Species display colors for a compartment.
   * @param compartmentIndex Compartment index.
   * @returns Species colors for compartment.
   */
  [[nodiscard]] const std::vector<QRgb> &
  getSpeciesColors(std::size_t compartmentIndex) const;
  /**
   * @brief Simulated time points.
   * @returns Time points.
   */
  [[nodiscard]] std::vector<double> getTimePoints() const;
  /**
   * @brief Average/min/max concentration summary.
   * @param timeIndex Time index.
   * @param compartmentIndex Compartment index.
   * @param speciesIndex Species index.
   * @returns Average/min/max summary.
   */
  [[nodiscard]] AvgMinMax getAvgMinMax(std::size_t timeIndex,
                                       std::size_t compartmentIndex,
                                       std::size_t speciesIndex) const;
  /**
   * @brief Concentrations in compartment voxel order.
   * @param timeIndex Time index.
   * @param compartmentIndex Compartment index.
   * @param speciesIndex Species index.
   * @returns Concentration values.
   */
  [[nodiscard]] std::vector<double> getConc(std::size_t timeIndex,
                                            std::size_t compartmentIndex,
                                            std::size_t speciesIndex) const;
  /**
   * @brief Concentrations in full image array order.
   * @param timeIndex Time index.
   * @param compartmentIndex Compartment index.
   * @param speciesIndex Species index.
   * @returns Concentration array in image order.
   */
  [[nodiscard]] std::vector<double>
  getConcArray(std::size_t timeIndex, std::size_t compartmentIndex,
               std::size_t speciesIndex) const;
  /**
   * @brief Write concentrations from one timepoint back into a model.
   * @param m Target model.
   * @param timeIndex Time index to apply.
   */
  void applyConcsToModel(model::Model &m, std::size_t timeIndex) const;
  /**
   * @brief Time derivatives in compartment voxel order.
   * @param compartmentIndex Compartment index.
   * @param speciesIndex Species index.
   * @returns Time derivative values.
   */
  [[nodiscard]] std::vector<double> getDcdt(std::size_t compartmentIndex,
                                            std::size_t speciesIndex) const;
  /**
   * @brief Time derivatives in full image array order.
   * @param compartmentIndex Compartment index.
   * @param speciesIndex Species index.
   * @returns Time derivative array in image order.
   */
  [[nodiscard]] std::vector<double>
  getDcdtArray(std::size_t compartmentIndex, std::size_t speciesIndex) const;
  /**
   * @brief Lower-order concentration value for adaptive RK.
   * @param compartmentIndex Compartment index.
   * @param speciesIndex Species index.
   * @param pixelIndex Pixel index.
   * @returns Lower-order concentration value.
   */
  [[nodiscard]] double getLowerOrderConc(std::size_t compartmentIndex,
                                         std::size_t speciesIndex,
                                         std::size_t pixelIndex) const;
  /**
   * @brief Render concentration image stack.
   * @param timeIndex Time index.
   * @param speciesToDraw Per-compartment species indices to draw.
   * @param normaliseOverAllTimepoints Normalize using all timepoints.
   * @param normaliseOverAllSpecies Normalize using all species.
   * @returns Concentration image stack.
   */
  [[nodiscard]] common::ImageStack
  getConcImage(std::size_t timeIndex,
               const std::vector<std::vector<std::size_t>> &speciesToDraw = {},
               bool normaliseOverAllTimepoints = false,
               bool normaliseOverAllSpecies = false) const;
  /**
   * @brief Species names for Python API.
   * @param compartmentIndex Compartment index.
   * @returns Species names.
   */
  [[nodiscard]] const std::vector<std::string> &
  getPyNames(std::size_t compartmentIndex) const;
  /**
   * @brief Concentrations formatted for Python API.
   * @param timeIndex Time index.
   * @param compartmentIndex Compartment index.
   * @returns Concentrations grouped by species.
   */
  [[nodiscard]] std::vector<std::vector<double>>
  getPyConcs(std::size_t timeIndex, std::size_t compartmentIndex) const;
  /**
   * @brief Time derivatives formatted for Python API.
   * @param compartmentIndex Compartment index.
   * @returns Time derivatives grouped by species.
   */
  [[nodiscard]] std::vector<std::vector<double>>
  getPyDcdts(std::size_t compartmentIndex) const;
  /**
   * @brief Number of completed timesteps.
   * @returns Completed timestep count.
   */
  [[nodiscard]] std::size_t getNCompletedTimesteps() const;
  /**
   * @brief Immutable simulation data storage.
   * @returns Simulation data.
   */
  [[nodiscard]] const SimulationData &getSimulationData() const;
  /**
   * @brief Returns ``true`` if solver is currently running.
   * @returns Running flag.
   */
  [[nodiscard]] bool getIsRunning() const;
  /**
   * @brief Returns ``true`` if stop has been requested.
   * @returns Stop-request flag.
   */
  [[nodiscard]] bool getIsStopping() const;
  /**
   * @brief Request graceful stop of running simulation.
   */
  void requestStop();
};

} // namespace simulate

} // namespace sme
