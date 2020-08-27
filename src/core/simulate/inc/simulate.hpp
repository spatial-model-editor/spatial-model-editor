// Simulator

#pragma once

#include "simulate_options.hpp"
#include <QImage>
#include <QRgb>
#include <QSize>
#include <cstddef>
#include <limits>
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

struct AvgMinMax {
  double avg = 0;
  double min = std::numeric_limits<double>::max();
  double max = 0;
};

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
  void initModel(const model::Model &model);
  void updateConcentrations(double t);

public:
  explicit Simulation(const model::Model &sbmlDoc,
                      SimulatorType simType = SimulatorType::DUNE,
                      const Options &options = {});
  ~Simulation();

  std::size_t doTimestep(double time);
  const std::string& errorMessage() const;
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
               bool normaliseOverWholeSim = false) const;
};

} // namespace simulate
