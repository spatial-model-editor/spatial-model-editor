// Simulator

#pragma once

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

struct IntegratorOptions {
  std::size_t order = 2;
  double maxRelErr = 0.01;
  double maxAbsErr = std::numeric_limits<double>::max();
  double maxTimestep = std::numeric_limits<double>::max();
};

class Simulation {
private:
  SimulatorType simulatorType;
  std::unique_ptr<BaseSim> simulator;
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
  void updateConcentrations(double t);

public:
  explicit Simulation(const model::Model &sbmlDoc,
                      SimulatorType simType = SimulatorType::DUNE,
                      std::size_t integratorOrder = 1);
  ~Simulation();
  IntegratorOptions getIntegratorOptions() const;
  void setIntegratorOptions(const IntegratorOptions &options);
  void setMaxThreads(std::size_t maxThreads);
  std::size_t getMaxThreads() const;

  std::size_t doTimestep(double time);
  std::string errorMessage() const;
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
