#include "simulate.hpp"
#include "dunesim.hpp"
#include "geometry.hpp"
#include "logger.hpp"
#include "mesh.hpp"
#include "model.hpp"
#include "pde.hpp"
#include "pixelsim.hpp"
#include "utils.hpp"
#include <QElapsedTimer>
#include <algorithm>
#include <limits>
#include <numeric>
#include <utility>

namespace sme::simulate {

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

void Simulation::initEvents(model::Model &model) {
  simEvents = {};
  double t0{0.0};
  if (!data->timePoints.empty()) {
    t0 = data->timePoints.back();
  }
  const auto &events{model.getEvents()};
  data->xmlModel = model.getXml().toStdString();
  std::vector<SimEvent> evs;
  for (const auto &id : events.getIds()) {
    double t{events.getTime(id)};
    SPDLOG_INFO("  - event '{}' at time {}", id.toStdString(), t);
    if (t >= t0) {
      if (auto iter{std::find_if(evs.begin(), evs.end(),
                                 [t](const auto &ev) { return ev.time == t; })};
          iter != evs.end()) {
        SPDLOG_INFO("    -> adding to existing SimEvent at time {}",
                    iter->time);
        iter->ids.push_back(id.toStdString());
      } else {
        evs.push_back({t, {id.toStdString()}});
        SPDLOG_INFO("    -> adding new SimEvent at time {}", evs.back().time);
      }
    } else {
      SPDLOG_INFO("    -> ignoring past event (t0 = {})", t0);
    }
  }
  std::sort(evs.begin(), evs.end(),
            [](const auto &a, const auto &b) { return a.time < b.time; });
  for (auto &ev : evs) {
    SPDLOG_INFO("Adding SimEvent at time {}", ev.time);
    for (const auto &id : ev.ids) {
      SPDLOG_INFO("  - '{}'", id);
    }
    simEvents.push(std::move(ev));
  }
  // add a null event at infinite time
  simEvents.push(
      {std::numeric_limits<double>::max(), {"null_infinite_time_event"}});
}

void Simulation::applyNextEvent() {
  const auto &ev{simEvents.front()};
  SPDLOG_INFO("Applying SimEvent at time {}", ev.time);
  // make new copy of model
  simModel = std::make_unique<sme::model::Model>();
  simModel->importSBMLString(data->xmlModel);
  // apply latest simulation concentrations to model
  applyConcsToModel(*simModel.get(), data->concentration.size() - 1);
  // apply events to model
  auto &events{simModel->getEvents()};
  for (const auto &id : ev.ids) {
    SPDLOG_INFO("  - event '{}'", id);
    events.applyEvent(id.c_str());
  }
  // export this model to xml for applying next event
  data->xmlModel = simModel->getXml().toStdString();
  // re-init simulator
  simulator.reset();
  if (simulatorType == SimulatorType::DUNE &&
      simModel->getGeometry().getMesh() != nullptr &&
      simModel->getGeometry().getMesh()->isValid()) {
    simulator = std::make_unique<DuneSim>(*simModel.get(), compartmentIds,
                                          simulatorOptions.dune);
  } else {
    simulator = std::make_unique<PixelSim>(*simModel.get(), compartmentIds,
                                           compartmentSpeciesIds,
                                           simulatorOptions.pixel);
  }
  // remove applied simEvent
  simEvents.pop();
}

static std::vector<AvgMinMax>
calculateAvgMinMax(const std::vector<double> &concs, std::size_t nSpecies,
                   std::size_t concPadding) {
  std::vector<AvgMinMax> avgMinMax(nSpecies);
  std::size_t stride{nSpecies + concPadding};
  for (std::size_t ix = 0; ix < concs.size() / stride; ++ix) {
    for (std::size_t is = 0; is < nSpecies; ++is) {
      auto &a = avgMinMax[is];
      double c = concs[ix * stride + is];
      a.avg += c;
      a.max = std::max(a.max, c);
      a.min = std::min(a.min, c);
    }
  }
  for (auto &a : avgMinMax) {
    a.avg /= static_cast<double>(concs.size()) / static_cast<double>(stride);
  }
  return avgMinMax;
}

void Simulation::updateConcentrations(double t) {
  SPDLOG_DEBUG("updating Concentrations at time {}", t);
  data->timePoints.push_back(t);
  data->concPadding.push_back(simulator->getConcentrationPadding());
  auto &c = data->concentration.emplace_back();
  c.reserve(compartments.size());
  auto &a = data->avgMinMax.emplace_back();
  a.reserve(compartments.size());
  if (data->concentrationMax.empty()) {
    auto &m = data->concentrationMax.emplace_back();
    for (std::size_t i = 0; i < compartments.size(); ++i) {
      std::size_t nSpecies = compartmentSpeciesIds[i].size();
      m.push_back(std::vector<double>(nSpecies, 0.0));
    }
  } else {
    data->concentrationMax.push_back(data->concentrationMax.back());
  }
  for (std::size_t compIndex = 0; compIndex < compartments.size();
       ++compIndex) {
    std::size_t nSpecies{compartmentSpeciesIds[compIndex].size()};
    const auto &compConcs{simulator->getConcentrations(compIndex)};
    c.push_back(compConcs);
    a.push_back(
        calculateAvgMinMax(compConcs, nSpecies, data->concPadding.back()));
    auto &maxS{data->concentrationMax.back()[compIndex]};
    for (std::size_t is = 0; is < nSpecies; ++is) {
      maxS[is] = std::max(maxS[is], a.back()[is].max);
    }
  }
}

Simulation::Simulation(model::Model &sbmlDoc, SimulatorType simType,
                       const Options &options)
    : simulatorType(simType),
      simulatorOptions(options), data{&sbmlDoc.getSimulationData()},
      imageSize(sbmlDoc.getGeometry().getImage().size()) {
  sme::model::Model *modelToSimulate{&sbmlDoc};
  if (data->timePoints.size() <= 1) {
    SPDLOG_INFO("starting new simulation");
    data->clear();
  } else {
    SPDLOG_INFO("re-importing model with {} timepoints",
                data->timePoints.size());
    simModel = std::make_unique<sme::model::Model>();
    simModel->importSBMLString(data->xmlModel);
    modelToSimulate = simModel.get();
  }
  initModel(sbmlDoc);
  initEvents(sbmlDoc);
  // init simulator
  if (simulatorType == SimulatorType::DUNE &&
      sbmlDoc.getGeometry().getMesh() != nullptr &&
      sbmlDoc.getGeometry().getMesh()->isValid()) {
    simulator = std::make_unique<DuneSim>(*modelToSimulate, compartmentIds,
                                          options.dune);
  } else {
    simulator = std::make_unique<PixelSim>(
        *modelToSimulate, compartmentIds, compartmentSpeciesIds, options.pixel);
  }
  if (simulator->errorMessage().empty()) {
    nCompletedTimesteps.store(data->timePoints.size());
    if (data->timePoints.empty()) {
      updateConcentrations(0);
      ++nCompletedTimesteps;
    }
  }
}

Simulation::~Simulation() = default;

std::size_t Simulation::doTimesteps(double time, std::size_t nSteps,
                                    double timeout_ms) {
  return doMultipleTimesteps({{nSteps, time}}, timeout_ms);
}

std::size_t Simulation::doMultipleTimesteps(
    const std::vector<std::pair<std::size_t, double>> &timesteps,
    double timeout_ms) {

  std::size_t nStepsTotal{0};
  for (const auto &timestep : timesteps) {
    nStepsTotal += timestep.first;
  }
  QElapsedTimer timer;
  timer.start();
  // ensure there is enough space that push_back won't cause a reallocation

  // todo: there should be a mutex/lock here in case reserve() reallocates
  // to avoid a user getting garbage data while the vector contents are moving
  data->reserve(data->size() + nStepsTotal);
  stopRequested.store(false);
  isRunning.store(true);
  std::size_t steps{0};
  double remaining_timeout_ms{-1.0};
  for (const auto &timestep : timesteps) {
    const auto [nSteps, time] = timestep;
    SPDLOG_INFO("doing {} timesteps of length {}", nSteps, time);
    for (std::size_t iStep = 0; iStep < nSteps; ++iStep) {
      if (timeout_ms >= 0.0) {
        remaining_timeout_ms =
            timeout_ms - static_cast<double>(timer.elapsed());
        if (remaining_timeout_ms < 0.0) {
          remaining_timeout_ms = 0.0;
        }
      }
      double nextEventTime{simEvents.front().time};
      // if an event would occur within this fraction of a timestep then apply
      // it now, rather than doing a minuscule extra simulation step
      constexpr double fractionTimestepEpsilon{1e-12};
      double currentTime{data->timePoints.back()};
      while (std::abs(currentTime - nextEventTime) / time <
             fractionTimestepEpsilon) {
        SPDLOG_INFO("t={}, applying event at {}", currentTime, nextEventTime);
        applyNextEvent();
        nextEventTime = simEvents.front().time;
      }
      double currentTimeStep{time};
      while ((currentTime + currentTimeStep - nextEventTime) / time >
             0.1 * fractionTimestepEpsilon) {
        // event would occur during this step: do a smaller sub-step until event
        double subTimeStep{nextEventTime - currentTime};
        SPDLOG_INFO("Sub-step of {} to apply event at {}", subTimeStep,
                    nextEventTime);
        steps += simulator->run(subTimeStep, remaining_timeout_ms);
        // update intermediate concentrations to be able to apply them to model
        updateConcentrations(currentTime + subTimeStep);
        // apply event
        applyNextEvent();
        nextEventTime = simEvents.front().time;
        // remove intermediate concentrations
        data->pop_back();
        currentTime += subTimeStep;
        currentTimeStep -= subTimeStep;
        SPDLOG_INFO("Remaining time step: {}", currentTimeStep);
      }
      steps += simulator->run(currentTimeStep, remaining_timeout_ms);
      if (!simulator->errorMessage().empty() || stopRequested.load()) {
        isRunning.store(false);
        return steps;
      }
      updateConcentrations(data->timePoints.back() + time);
      ++nCompletedTimesteps;
    }
  }
  // write final concentrations to xml Model in data
  sme::model::Model finalModel;
  finalModel.importSBMLString(data->xmlModel);
  applyConcsToModel(finalModel, nCompletedTimesteps - 1);
  data->xmlModel = finalModel.getXml().toStdString();
  isRunning.store(false);
  return steps;
}

const std::string &Simulation::errorMessage() const {
  return simulator->errorMessage();
}

const QImage &Simulation::errorImage() const { return simulator->errorImage(); }

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
  return data->timePoints;
}

const AvgMinMax &Simulation::getAvgMinMax(std::size_t timeIndex,
                                          std::size_t compartmentIndex,
                                          std::size_t speciesIndex) const {
  return data->avgMinMax[timeIndex][compartmentIndex][speciesIndex];
}

std::vector<double> Simulation::getConc(std::size_t timeIndex,
                                        std::size_t compartmentIndex,
                                        std::size_t speciesIndex) const {
  std::vector<double> c;
  const auto &compConc = data->concentration[timeIndex][compartmentIndex];
  std::size_t nPixels = compartments[compartmentIndex]->nPixels();
  std::size_t nSpecies = compartmentSpeciesIds[compartmentIndex].size();
  c.reserve(nPixels);
  std::size_t stride{nSpecies + data->concPadding[timeIndex]};
  for (std::size_t ix = 0; ix < nPixels; ++ix) {
    c.push_back(compConc[ix * stride + speciesIndex]);
  }
  return c;
}

std::vector<double> Simulation::getConcArray(std::size_t timeIndex,
                                             std::size_t compartmentIndex,
                                             std::size_t speciesIndex) const {
  std::vector<double> c(
      static_cast<std::size_t>(imageSize.width() * imageSize.height()), 0.0);
  const auto &compConc = data->concentration[timeIndex][compartmentIndex];
  const auto &comp = compartments[compartmentIndex];
  std::size_t nPixels = comp->nPixels();
  std::size_t nSpecies = compartmentSpeciesIds[compartmentIndex].size();
  std::size_t stride{nSpecies + data->concPadding[timeIndex]};
  for (std::size_t ix = 0; ix < nPixels; ++ix) {
    const auto &point = comp->getPixel(ix);
    auto arrayIndex{static_cast<std::size_t>(
        point.x() + imageSize.width() * (imageSize.height() - 1 - point.y()))};
    c[arrayIndex] = compConc[ix * stride + speciesIndex];
  }
  return c;
}

void Simulation::applyConcsToModel(model::Model &model,
                                   std::size_t timeIndex) const {
  for (std::size_t iCompartment = 0; iCompartment < compartmentIds.size();
       ++iCompartment) {
    const auto &speciesIds{getSpeciesIds(iCompartment)};
    for (std::size_t iSpecies = 0; iSpecies < speciesIds.size(); ++iSpecies) {
      model.getSpecies().setSampledFieldConcentration(
          speciesIds[iSpecies].c_str(),
          getConcArray(timeIndex, iCompartment, iSpecies));
    }
  }
}

std::vector<double> Simulation::getDcdt(std::size_t compartmentIndex,
                                        std::size_t speciesIndex) const {
  std::vector<double> c;
  if (auto *s = dynamic_cast<PixelSim *>(simulator.get()); s != nullptr) {
    const auto &compDcdt = s->getDcdt(compartmentIndex);
    std::size_t nPixels = compartments[compartmentIndex]->nPixels();
    std::size_t nSpecies = compartmentSpeciesIds[compartmentIndex].size();
    c.reserve(nPixels);
    std::size_t stride{nSpecies + data->concPadding.back()};
    for (std::size_t ix = 0; ix < nPixels; ++ix) {
      c.push_back(compDcdt[ix * stride + speciesIndex]);
    }
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
  constexpr double minimumNonzeroConc{100.0 *
                                      std::numeric_limits<double>::min()};
  const auto *speciesIndices = &speciesToDraw;
  // default to drawing all species if not specified
  if (speciesToDraw.empty()) {
    speciesIndices = &compartmentSpeciesIndices;
  }
  // calculate normalisation for each species
  auto maxConcs{data->concentrationMax
                    [nCompletedTimesteps.load(std::memory_order_seq_cst) - 1]};
  if (!normaliseOverAllTimepoints) {
    // get max for each species at this timepoint
    for (std::size_t ic = 0; ic < compartments.size(); ++ic) {
      for (std::size_t is : (*speciesIndices)[ic]) {
        maxConcs[ic][is] = data->avgMinMax[timeIndex][ic][is].max;
      }
    }
  }
  if (normaliseOverAllSpecies) {
    // normalise over max of all visible species
    double maxC{minimumNonzeroConc};
    for (std::size_t ic = 0; ic < compartments.size(); ++ic) {
      for (std::size_t is : (*speciesIndices)[ic]) {
        maxC = std::max(maxC, maxConcs[ic][is]);
      }
    }
    for (auto &c : maxConcs) {
      std::fill(c.begin(), c.end(), maxC);
    }
  }
  // apply minimum (avoid dividing by zero)
  for (std::size_t ic = 0; ic < compartments.size(); ++ic) {
    for (std::size_t is : (*speciesIndices)[ic]) {
      if (maxConcs[ic][is] < minimumNonzeroConc) {
        maxConcs[ic][is] = minimumNonzeroConc;
      }
    }
  }
  QImage img(imageSize, QImage::Format_ARGB32_Premultiplied);
  img.fill(qRgba(0, 0, 0, 0));
  // iterate over compartments
  for (std::size_t ic = 0; ic < compartments.size(); ++ic) {
    const auto &pixels{compartments[ic]->getPixels()};
    const auto &conc{data->concentration[timeIndex][ic]};
    std::size_t nSpecies = compartmentSpeciesIds[ic].size();
    std::size_t stride{nSpecies + data->concPadding[timeIndex]};
    for (std::size_t ix = 0; ix < pixels.size(); ++ix) {
      const QPoint &p{pixels[ix]};
      int r = 0;
      int g = 0;
      int b = 0;
      for (std::size_t is : (*speciesIndices)[ic]) {
        double c = conc[ix * stride + is] / maxConcs[ic][is];
        const auto &col = compartmentSpeciesColors[ic][is];
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

std::pair<std::map<std::string, std::vector<std::vector<double>>>,
          std::map<std::string, std::vector<std::vector<double>>>>
Simulation::getPyConcs(std::size_t timeIndex) const {
  using PyConc = std::vector<std::vector<double>>;
  std::pair<std::map<std::string, PyConc>, std::map<std::string, PyConc>> pair;
  auto &[pyConcs, pyDcdts] = pair;
  PyConc zeros = PyConc(
      static_cast<std::size_t>(imageSize.height()),
      std::vector<double>(static_cast<std::size_t>(imageSize.width()), 0.0));
  // start with zero concentration everywhere for all species
  std::vector<std::vector<PyConc>> vecPyConcs;
  std::vector<std::vector<PyConc>> vecPyDcdts;
  vecPyConcs.reserve(compartmentSpeciesIds.size());
  vecPyDcdts.reserve(compartmentSpeciesIds.size());
  for (const auto &speciesIds : compartmentSpeciesIds) {
    vecPyConcs.emplace_back(speciesIds.size(), zeros);
    vecPyDcdts.emplace_back(speciesIds.size(), zeros);
  }
  // insert concentration for each pixel & species
  for (std::size_t ci = 0; ci < compartmentSpeciesIds.size(); ++ci) {
    const auto &pixels = compartments[ci]->getPixels();
    const auto &conc = data->concentration[timeIndex][ci];
    const std::vector<double> *dcdt{nullptr};
    if (auto *s = dynamic_cast<PixelSim *>(simulator.get()); s != nullptr) {
      dcdt = &(s->getDcdt(ci));
    }
    std::size_t nSpecies = compartmentSpeciesIds[ci].size();
    std::size_t stride{nSpecies + data->concPadding[timeIndex]};
    for (std::size_t ix = 0; ix < pixels.size(); ++ix) {
      const QPoint &p = pixels[ix];
      auto x = static_cast<std::size_t>(p.x());
      auto y = static_cast<std::size_t>(p.y());
      for (std::size_t is : compartmentSpeciesIndices[ci]) {
        vecPyConcs[ci][is][y][x] = conc[ix * stride + is];
        if (dcdt != nullptr) {
          vecPyDcdts[ci][is][y][x] = (*dcdt)[ix * stride + is];
        }
      }
    }
  }
  // construct map from species name to PyConc
  for (std::size_t ci = 0; ci < compartmentSpeciesIds.size(); ++ci) {
    for (std::size_t is : compartmentSpeciesIndices[ci]) {
      pyConcs[compartmentSpeciesNames[ci][is]] = std::move(vecPyConcs[ci][is]);
      pyDcdts[compartmentSpeciesNames[ci][is]] = std::move(vecPyDcdts[ci][is]);
    }
  }
  return pair;
}

std::size_t Simulation::getNCompletedTimesteps() const {
  return nCompletedTimesteps;
}

const SimulationData &Simulation::getSimulationData() const { return *data; }

bool Simulation::getIsRunning() const { return isRunning.load(); }

bool Simulation::getIsStopping() const { return stopRequested.load(); }

void Simulation::requestStop() {
  stopRequested.store(true);
  simulator->requestStop();
}

} // namespace sme::simulate
