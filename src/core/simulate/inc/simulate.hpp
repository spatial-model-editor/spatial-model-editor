// Simulator

#pragma once

#include "simulate_options.hpp"
#include <QImage>
#include <QRgb>
#include <QSize>
#include <atomic>
#include <cstddef>
#include <limits>
#include <map>
#include <memory>
#include <string>
#include <vector>

namespace model {
class Model;
}

namespace geometry {
class Compartment;
}

namespace simulate {

class BaseSim;
enum class SimulatorType { DUNE, Pixel };

class Simulation {
private:
  std::unique_ptr<BaseSim> simulator;
  SimulatorType simulatorType;
  DuneOptions duneOptions;
  PixelOptions pixelOptions;
  std::vector<const geometry::Compartment *> compartments;
  std::vector<std::string> compartmentIds;
  // compartment->species
  std::vector<std::vector<std::string>> compartmentSpeciesIds;
  std::vector<std::vector<std::string>> compartmentSpeciesNames;
  std::vector<std::vector<std::size_t>> compartmentSpeciesIndices;
  std::vector<std::vector<QRgb>> compartmentSpeciesColors;
  std::vector<double> timePoints;
  // time->compartment->(ix->species)
  std::vector<std::vector<std::vector<double>>> concentration;
  // time->compartment->species
  std::vector<std::vector<std::vector<AvgMinMax>>> avgMinMax;
  // compartment->species
  std::vector<std::vector<double>> maxConcWholeSimulation;
  QSize imageSize;
  std::atomic<bool> isRunning{false};
  std::atomic<bool> stopRequested{false};
  std::atomic<std::size_t> nCompletedTimesteps{0};
  void initModel(const model::Model &model);
  void updateConcentrations(double t);

public:
  explicit Simulation(const model::Model &sbmlDoc,
                      SimulatorType simType = SimulatorType::DUNE,
                      const Options &options = {});
  ~Simulation();

  std::size_t doTimesteps(double time, std::size_t nSteps = 1);
  const std::string &errorMessage() const;
  const std::vector<std::string> &getCompartmentIds() const;
  const std::vector<std::string> &
  getSpeciesIds(std::size_t compartmentIndex) const;
  const std::vector<QRgb> &getSpeciesColors(std::size_t compartmentIndex) const;
  const std::vector<double> &getTimePoints() const;
  const AvgMinMax &getAvgMinMax(std::size_t timeIndex,
                                std::size_t compartmentIndex,
                                std::size_t speciesIndex) const;
  std::vector<double> getConc(std::size_t timeIndex,
                              std::size_t compartmentIndex,
                              std::size_t speciesIndex) const;
  double getLowerOrderConc(std::size_t compartmentIndex,
                           std::size_t speciesIndex,
                           std::size_t pixelIndex) const;
  QImage
  getConcImage(std::size_t timeIndex,
               const std::vector<std::vector<std::size_t>> &speciesToDraw = {},
               bool normaliseOverAllTimepoints = false,
               bool normaliseOverAllSpecies = false) const;
  // map from name to vec<vec<double>> species concentration for python bindings
  std::map<std::string, std::vector<std::vector<double>>>
  getPyConcs(std::size_t timeIndex) const;
  std::size_t getNCompletedTimesteps() const;
  bool getIsRunning() const;
  bool getIsStopping() const;
  void requestStop();
};

} // namespace simulate
