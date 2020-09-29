#include "simulate.hpp"
#include "dunesim.hpp"
#include "geometry.hpp"
#include "logger.hpp"
#include "mesh.hpp"
#include "model.hpp"
#include "pde.hpp"
#include "pixelsim.hpp"
#include "utils.hpp"
#include <algorithm>
#include <numeric>
#include <utility>

namespace simulate {

void Simulation::initModel(const model::Model &model) {
  // get compartments with interacting species, name & colour of each species
  for (const auto &compartmentId : model.getCompartments().getIds()) {
    std::vector<std::string> sIds;
    std::vector<QRgb> cols;
    const geometry::Compartment *comp = nullptr;
    for (const auto &s : model.getSpecies().getIds(compartmentId)) {
      if (!model.getSpecies().getIsConstant(s)) {
        sIds.push_back(s.toStdString());
        const auto &field = model.getSpecies().getField(s);
        cols.push_back(field->getColour());
        comp = field->getCompartment();
      }
    }
    if (!sIds.empty()) {
      maxConcWholeSimulation.emplace_back(sIds.size(), 0.0);
      auto sIndices = std::vector<std::size_t>(sIds.size());
      std::iota(sIndices.begin(), sIndices.end(), 0);
      compartmentIds.push_back(compartmentId.toStdString());
      compartmentSpeciesIds.push_back(std::move(sIds));
      auto &names = compartmentSpeciesNames.emplace_back();
      for (const auto &id : compartmentSpeciesIds.back()) {
        names.push_back(model.getSpecies().getName(id.c_str()).toStdString());
      }
      compartmentSpeciesIndices.push_back(std::move(sIndices));
      compartmentSpeciesColors.push_back(std::move(cols));
      compartments.push_back(comp);
    }
  }
}

static std::vector<AvgMinMax>
calculateAvgMinMax(const std::vector<double> &concs, std::size_t nSpecies) {
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
    a.avg /= static_cast<double>(concs.size()) / static_cast<double>(nSpecies);
  }
  return avgMinMax;
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

Simulation::Simulation(const model::Model &sbmlDoc, SimulatorType simType,
                       const Options &options)
    : simulatorType(simType),
      imageSize(sbmlDoc.getGeometry().getImage().size()) {
  initModel(sbmlDoc);
  // init simulator
  if (simulatorType == SimulatorType::DUNE &&
      sbmlDoc.getGeometry().getMesh() != nullptr &&
      sbmlDoc.getGeometry().getMesh()->isValid()) {
    simulator = std::make_unique<DuneSim>(sbmlDoc, compartmentIds,
                                          compartmentSpeciesIds, options.dune);
  } else {
    simulator = std::make_unique<PixelSim>(
        sbmlDoc, compartmentIds, compartmentSpeciesIds, options.pixel);
  }
  if (simulator->errorMessage().empty()) {
    updateConcentrations(0);
  }
}

Simulation::~Simulation() = default;

std::size_t Simulation::doTimestep(double time) {
  SPDLOG_DEBUG("integrating for time {}", time);
  std::size_t steps = simulator->run(time);
  updateConcentrations(timePoints.back() + time);
  return steps;
}

const std::string &Simulation::errorMessage() const {
  return simulator->errorMessage();
}

const std::vector<std::string> &Simulation::getCompartmentIds() const {
  return compartmentIds;
}

const std::vector<std::string> &
Simulation::getSpeciesIds(std::size_t compartmentIndex) const {
  return compartmentSpeciesIds[compartmentIndex];
}

const std::vector<QRgb> &
Simulation::getSpeciesColors(std::size_t compartmentIndex) const {
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
  if (auto *s = dynamic_cast<PixelSim *>(simulator.get()); s != nullptr) {
    return s->getLowerOrderConcentration(compartmentIndex, speciesIndex,
                                         pixelIndex);
  }
  return 0;
}

QImage Simulation::getConcImage(
    std::size_t timeIndex,
    const std::vector<std::vector<std::size_t>> &speciesToDraw,
    bool normaliseOverAllTimepoints, bool normaliseOverAllSpecies) const {
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
    if (normaliseOverAllTimepoints) {
      maxConcs = maxConcWholeSimulation[compIndex];
    } else {
      for (std::size_t is : (*speciesIndices)[compIndex]) {
        double m = avgMinMax[timeIndex][compIndex][is].max;
        constexpr double minimumNonzeroConc{1e-30};
        maxConcs[is] = m > minimumNonzeroConc ? m : 1.0;
      }
    }
    if (normaliseOverAllSpecies) {
      double maxConc{utils::max(maxConcs)};
      std::fill(maxConcs.begin(), maxConcs.end(), maxConc);
    }
    for (std::size_t ix = 0; ix < pixels.size(); ++ix) {
      const QPoint &p = pixels[ix];
      int r = 0;
      int g = 0;
      int b = 0;
      for (std::size_t is : (*speciesIndices)[compIndex]) {
        double c = conc[ix * nSpecies + is] / maxConcs[is];
        const auto &col = compartmentSpeciesColors[compIndex][is];
        r += static_cast<int>(qRed(col) * c);
        g += static_cast<int>(qGreen(col) * c);
        b += static_cast<int>(qBlue(col) * c);
      }
      r = r < 256 ? r : 255;
      g = g < 256 ? g : 255;
      b = b < 256 ? b : 255;
      img.setPixel(p, qRgb(r, g, b));
    }
  }
  return img;
}

std::map<std::string, std::vector<std::vector<double>>>
Simulation::getPyConcs(std::size_t timeIndex) const {
  using PyConc = std::vector<std::vector<double>>;
  std::map<std::string, PyConc> pyConcs;
  PyConc zeros = PyConc(
      static_cast<std::size_t>(imageSize.height()),
      std::vector<double>(static_cast<std::size_t>(imageSize.width()), 0.0));
  // start with zero concentration everywhere for all species
  std::vector<std::vector<PyConc>> vecPyConcs;
  vecPyConcs.reserve(compartmentSpeciesIds.size());
  for (const auto &speciesIds : compartmentSpeciesIds) {
    vecPyConcs.emplace_back(speciesIds.size(), zeros);
  }
  // insert concentration for each pixel & species
  for (std::size_t ci = 0; ci < compartmentSpeciesIds.size(); ++ci) {
    const auto &pixels = compartments[ci]->getPixels();
    const auto &conc = concentration[timeIndex][ci];
    std::size_t nSpecies = compartmentSpeciesIds[ci].size();
    for (std::size_t ix = 0; ix < pixels.size(); ++ix) {
      const QPoint &p = pixels[ix];
      auto x = static_cast<std::size_t>(p.x());
      auto y = static_cast<std::size_t>(p.y());
      for (std::size_t is : compartmentSpeciesIndices[ci]) {
        vecPyConcs[ci][is][y][x] = conc[ix * nSpecies + is];
      }
    }
  }
  // construct map from species name to PyConc
  for (std::size_t ci = 0; ci < compartmentSpeciesIds.size(); ++ci) {
    for (std::size_t is : compartmentSpeciesIndices[ci]) {
      pyConcs[compartmentSpeciesNames[ci][is]] = std::move(vecPyConcs[ci][is]);
    }
  }
  return pyConcs;
}

} // namespace simulate
