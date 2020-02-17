#include "simulate.hpp"

#include "dunesim.hpp"
#include "geometry.hpp"
#include "logger.hpp"
#include "pde.hpp"
#include "pixelsim.hpp"
#include "sbml.hpp"
#include "utils.hpp"

namespace simulate {

Simulation::Simulation(const sbml::SbmlDocWrapper &sbmlDoc,
                       SimulatorType simType)
    : simulatorType(simType), imageSize(sbmlDoc.getCompartmentImage().size()) {
  // init simulator
  if (simulatorType == SimulatorType::DUNE) {
    simulator = std::make_unique<sim::DuneSim>(sbmlDoc);
  } else {
    simulator = std::make_unique<sim::PixelSim>(sbmlDoc);
  }
  // get compartments with interacting species, name & colour of each species
  for (const auto &compartmentId : sbmlDoc.compartments) {
    std::vector<std::string> sIds;
    std::vector<QColor> cols;
    const geometry::Compartment *comp = nullptr;
    for (const auto &s : sbmlDoc.species.at(compartmentId)) {
      if (!sbmlDoc.getIsSpeciesConstant(s.toStdString())) {
        sIds.push_back(s.toStdString());
        const auto &field = sbmlDoc.mapSpeciesIdToField.at(s);
        cols.push_back(field.colour);
        comp = field.geometry;
      }
    }
    if (!sIds.empty()) {
      maxConcWholeSimulation.push_back(std::vector<double>(sIds.size(), 0.0));
      auto sIndices = std::vector<std::size_t>(sIds.size());
      std::iota(sIndices.begin(), sIndices.end(), 0);
      compartmentIds.push_back(compartmentId.toStdString());
      compartmentSpeciesIds.push_back(std::move(sIds));
      compartmentSpeciesIndices.push_back(std::move(sIndices));
      compartmentSpeciesColors.push_back(std::move(cols));
      compartments.push_back(comp);
    }
  }
  updateConcentrations(0);
}

Simulation::~Simulation() = default;

IntegratorOptions Simulation::getIntegratorOptions() const {
  IntegratorOptions options;
  options.order = simulator->getIntegrationOrder();
  options.maxRelErr = simulator->getIntegratorError().rel;
  options.maxAbsErr = simulator->getIntegratorError().abs;
  options.maxTimestep = simulator->getMaxDt();
  return options;
}

void Simulation::setIntegratorOptions(const IntegratorOptions &options) {
  simulator->setIntegrationOrder(options.order);
  auto e = simulator->getIntegratorError();
  e.rel = options.maxRelErr;
  e.abs = options.maxAbsErr;
  simulator->setIntegratorError(e);
  simulator->setMaxDt(options.maxTimestep);
}

static std::vector<AvgMinMax> calculateAvgMinMax(
    const std::vector<double> &concs, std::size_t nSpecies) {
  std::vector<AvgMinMax> avgMinMax(nSpecies);
  for (std::size_t ix = 0; ix < concs.size() / nSpecies; ++ix) {
    for (std::size_t is = 0; is < nSpecies; ++is) {
      auto &a = avgMinMax[is];
      double c = concs[ix * nSpecies + is];
      a.avg += c;
      a.max = std::max(a.max, c);
      a.min = std::min(a.min, c);
    }
  }
  for (auto &a : avgMinMax) {
    a.avg /= static_cast<double>(concs.size() / nSpecies);
  }
  return avgMinMax;
}

std::size_t Simulation::doTimestep(double time) {
  SPDLOG_DEBUG("integrating for time {}", time);
  SPDLOG_DEBUG("  - max rel local err {}", simulator->getIntegratorError().rel);
  SPDLOG_DEBUG("  - max abs local err {}", simulator->getIntegratorError().abs);
  SPDLOG_DEBUG("  - max stepsize {}", simulator->getMaxDt());
  std::size_t steps = simulator->run(time);
  updateConcentrations(timePoints.back() + time);
  return steps;
}

void Simulation::updateConcentrations(double t) {
  SPDLOG_DEBUG("updating Concentrations at time {}", t);
  timePoints.push_back(t);
  auto &c = concentration.emplace_back();
  c.reserve(compartments.size());
  auto &a = avgMinMax.emplace_back();
  a.reserve(compartments.size());
  for (std::size_t compIndex = 0; compIndex < compartments.size();
       ++compIndex) {
    std::size_t nSpecies = compartmentSpeciesIds[compIndex].size();
    const auto &compConcs = simulator->getConcentrations(compIndex);
    c.push_back(compConcs);
    a.push_back(calculateAvgMinMax(compConcs, nSpecies));
    auto &maxC = maxConcWholeSimulation[compIndex];
    for (std::size_t is = 0; is < nSpecies; ++is) {
      maxC[is] = std::max(maxC[is], a.back()[is].max);
    }
  }
}

const std::vector<std::string> &Simulation::getCompartmentIds() const {
  return compartmentIds;
}

const std::vector<std::string> &Simulation::getSpeciesIds(
    std::size_t compartmentIndex) const {
  return compartmentSpeciesIds[compartmentIndex];
}

const std::vector<QColor> &Simulation::getSpeciesColors(
    std::size_t compartmentIndex) const {
  return compartmentSpeciesColors[compartmentIndex];
}

const std::vector<double> &Simulation::getTimePoints() const {
  return timePoints;
}

const AvgMinMax &Simulation::getAvgMinMax(std::size_t timeIndex,
                                          std::size_t compartmentIndex,
                                          std::size_t speciesIndex) const {
  return avgMinMax[timeIndex][compartmentIndex][speciesIndex];
}

std::vector<double> Simulation::getConc(std::size_t timeIndex,
                                        std::size_t compartmentIndex,
                                        std::size_t speciesIndex) const {
  std::vector<double> c;
  const auto &compConc = concentration[timeIndex][compartmentIndex];
  std::size_t nPixels = compartments[compartmentIndex]->nPixels();
  std::size_t nSpecies = compartmentSpeciesIds[compartmentIndex].size();
  c.reserve(nPixels);
  for (std::size_t ix = 0; ix < nPixels; ++ix) {
    c.push_back(compConc[ix * nSpecies + speciesIndex]);
  }
  return c;
}

double Simulation::getLowerOrderConc(std::size_t compartmentIndex,
                                     std::size_t speciesIndex,
                                     std::size_t pixelIndex) const {
  return simulator->getLowerOrderConcentration(compartmentIndex, speciesIndex,
                                               pixelIndex);
}

QImage Simulation::getConcImage(
    std::size_t timeIndex,
    const std::vector<std::vector<std::size_t>> &speciesToDraw,
    bool normaliseOverWholeSim) const {
  if (compartments.empty()) {
    return QImage();
  }
  const auto *speciesIndices = &speciesToDraw;
  // default to drawing all species if not specified
  if (speciesToDraw.empty()) {
    speciesIndices = &compartmentSpeciesIndices;
  }
  QImage img(imageSize, QImage::Format_ARGB32_Premultiplied);
  img.fill(qRgba(0, 0, 0, 0));
  // iterate over compartments
  for (std::size_t compIndex = 0; compIndex < compartments.size();
       ++compIndex) {
    const auto &pixels = compartments[compIndex]->getPixels();
    const auto &conc = concentration[timeIndex][compIndex];
    std::size_t nSpecies = compartmentSpeciesIds[compIndex].size();
    // normalise species concentration:
    // max value of each species = max colour intensity
    // (with lower bound, so constant zero is still zero)
    std::vector<double> maxConcs(compartmentSpeciesIds[compIndex].size(), 1.0);
    if (normaliseOverWholeSim) {
      maxConcs = maxConcWholeSimulation[compIndex];
    } else {
      for (std::size_t is : (*speciesIndices)[compIndex]) {
        double m = avgMinMax[timeIndex][compIndex][is].max;
        maxConcs[is] = m > 1e-30 ? m : 1.0;
      }
    }
    for (std::size_t ix = 0; ix < pixels.size(); ++ix) {
      const QPoint &p = pixels[ix];
      int r = 0;
      int g = 0;
      int b = 0;
      for (std::size_t is : (*speciesIndices)[compIndex]) {
        double c = conc[ix * nSpecies + is] / maxConcs[is];
        const auto &col = compartmentSpeciesColors[compIndex][is];
        r += static_cast<int>(col.red() * c);
        g += static_cast<int>(col.green() * c);
        b += static_cast<int>(col.blue() * c);
      }
      r = r < 256 ? r : 255;
      g = g < 256 ? g : 255;
      b = b < 256 ? b : 255;
      img.setPixel(p, qRgb(r, g, b));
    }
  }
  return img;
}
}  // namespace simulate
