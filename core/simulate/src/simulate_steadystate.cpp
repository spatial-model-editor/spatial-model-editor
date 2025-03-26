#include "sme/simulate_steadystate.hpp"
#include "dunesim.hpp"
#include "pixelsim.hpp"
#include "sme/duneconverter.hpp"
#include "sme/simulate_options.hpp"
#include <QElapsedTimer>
#include <QFile>
#include <QImage>
#include <QPainter>
#include <algorithm>
#include <cmath>
#include <dune/common/exceptions.hh>
#include <limits>
#include <oneapi/tbb/global_control.h>
#include <oneapi/tbb/info.h>
#include <spdlog/spdlog.h>
#include <vector>

namespace sme::simulate {

//////////////////////////////////////////////////////////////////////////////////
// lifecycle
SteadyStateSimulation::SteadyStateSimulation(
    sme::model::Model &model, sme::simulate::SimulatorType type,
    double tolerance, std::size_t steps_to_convergence,
    SteadystateConvergenceMode convergence_mode, std::size_t timeout_ms,
    double dt)
    : m_has_converged(false), m_model(model),
      m_convergence_tolerance(tolerance), m_steps_below_tolerance(0),
      m_steps_to_convergence(steps_to_convergence), m_timeout_ms(timeout_ms),
      m_stop_mode(convergence_mode), m_data(), m_compartmentIdxs(),
      m_compartmentIds(), m_compartmentSpeciesIds(),
      m_compartmentSpeciesColors(), m_dt(dt), m_stopRequested(false) {
  m_model.getSimulationSettings().simulatorType = type;
  initModel();
  selectSimulator();
}

//////////////////////////////////////////////////////////////////////////////////
// helper functions for solver setup
void SteadyStateSimulation::initModel() {
  int i = 0;
  m_compartmentIds.clear();
  m_compartmentSpeciesIds.clear();
  m_compartmentSpeciesColors.clear();
  m_compartmentIdxs.clear();
  m_compartments.clear();

  // TODO: compare this to simulate::initModel and check that I didn't mess up
  // the ordering
  for (const auto &compartmentId : m_model.getCompartments().getIds()) {
    m_compartmentIdxs.push_back(i);
    m_compartmentIds.push_back(compartmentId.toStdString());
    m_compartmentSpeciesIds.push_back({});
    m_compartmentSpeciesColors.push_back({});
    auto comp = m_model.getCompartments().getCompartment(compartmentId);
    m_compartments.push_back(comp);
    for (const auto &s : m_model.getSpecies().getIds(compartmentId)) {
      if (!m_model.getSpecies().getIsConstant(s)) {
        m_compartmentSpeciesIds[i].push_back(s.toStdString());
        const auto &field = m_model.getSpecies().getField(s);
        m_compartmentSpeciesColors[i].push_back(field->getColor());
      }
    }
    i++;
  }
}

void SteadyStateSimulation::selectSimulator() {

  if (m_model.getSimulationSettings().simulatorType == SimulatorType::DUNE &&
      m_model.getGeometry().getIsMeshValid())
    m_simulator = std::make_unique<DuneSim>(m_model, m_compartmentIds);
  else {
    m_simulator = std::make_unique<PixelSim>(m_model, m_compartmentIds,
                                             m_compartmentSpeciesIds);
  }
}

void SteadyStateSimulation::resetSolver() {
  m_simulator = nullptr;
  m_steps_below_tolerance = 0;
  m_has_converged.store(false);
  initModel();
  selectSimulator();
}

double SteadyStateSimulation::computeStoppingCriterion(
    const std::vector<double> &c_old, const std::vector<double> &c_new) {
  double sum_squared_dcdt = 0.0;
  double sum_squared_c = 0.0;

  for (size_t i = 0; i < c_new.size(); ++i) {
    double dcdt = (c_new[i] - c_old[i]) / std::max(m_dt, 1e-12);
    // Sum squares for L2 norm calculations
    sum_squared_dcdt += dcdt * dcdt;
    sum_squared_c += c_new[i] * c_new[i];
  }

  double c_norm = std::sqrt(sum_squared_c);
  double dcdt_norm = std::sqrt(sum_squared_dcdt);
  if (m_stop_mode == SteadystateConvergenceMode::relative) {
    dcdt_norm = dcdt_norm / std::max(c_norm, 1e-12);
  }

  SPDLOG_DEBUG("    - c_norm: {}, dcdt_norm: {}", c_norm, dcdt_norm);

  return dcdt_norm;
}

//////////////////////////////////////////////////////////////////////////////////
// helpers for data
void SteadyStateSimulation::recordData(double timestep, double error) {
  common::ImageStack concImgStack = m_model.getGeometry().getImages();
  m_data.store(
      std::make_shared<SteadyStateData>(timestep, error, concImgStack));
}

void SteadyStateSimulation::resetData() {
  m_data.store(std::make_shared<SteadyStateData>());
}

//////////////////////////////////////////////////////////////////////////////////
// helper functions to run stuff

void SteadyStateSimulation::runPixel(double time) {
  m_simulator->setCurrentErrormessage("");

  QElapsedTimer timer;

  std::vector<double> c_old = getConcentrations();
  std::vector<double> c_new;
  c_new.reserve(c_old.capacity());

  // do timesteps until we reach t
  double tNow = 0;
  constexpr double relativeTolerance = 1e-12;

  timer.start();
  while (tNow + time * relativeTolerance < time) {

    m_simulator->run(m_dt, m_timeout_ms,
                     [&]() { return m_simulator->getStopRequested(); });

    c_new = getConcentrations();

    auto current_error = computeStoppingCriterion(c_old, c_new);

    c_old.swap(c_new);

    if (std::isnan(current_error) or std::isinf(current_error)) {
      m_simulator->setCurrentErrormessage(
          "Simulation failed: NaN  of Inf detected in norm");
      SPDLOG_DEBUG(m_simulator->errorMessage());
      m_simulator->setStopRequested(true);
      break;
    }

    SPDLOG_DEBUG("  - time: {}, current error: {}, tolerance: {}, "
                 "steps_below_tolerance: "
                 "{}",
                 tNow, current_error, m_convergence_tolerance,
                 m_steps_below_tolerance);

    if (current_error < m_convergence_tolerance) {
      m_steps_below_tolerance++;
    } else {
      m_steps_below_tolerance = 0;
    }

    if (m_steps_below_tolerance >= m_steps_to_convergence) {
      m_has_converged.store(true);
      m_simulator->setStopRequested(true);
      SPDLOG_CRITICAL("Simulation has converged");
      break;
    }
    SPDLOG_DEBUG(" timeout situation: {}, {}", m_timeout_ms,
                 static_cast<double>(timer.elapsed()));
    if (m_timeout_ms >= 0.0 &&
        static_cast<double>(timer.elapsed()) >= m_timeout_ms) {
      SPDLOG_DEBUG("Simulation timeout: requesting stop");
      m_simulator->setStopRequested(true);
      m_simulator->setCurrentErrormessage("Simulation timed out");
      break;
    }

    if (m_simulator->getStopRequested()) {
      m_simulator->setCurrentErrormessage("Simulation stopped early");
      SPDLOG_DEBUG("Simulation timeout or stopped early");
      break;
    }

    if (m_stopRequested) {
      break;
    }

    tNow += m_dt;

    recordData(tNow, current_error);
  }
}

void SteadyStateSimulation::runDune(double time) {
  QElapsedTimer timer;

  timer.start();

  double tNow = 0.;

  double relativeTolerance = 1e-12;

  std::vector<double> c_old = getConcentrations();

  std::vector<double> c_new;
  c_new.reserve(c_old.capacity());
  c_new.resize(c_old.size());

  // use dt here to avoid too long intervals for checking the stopping criterion
  while (tNow + time * relativeTolerance < time) {
    try {
      m_simulator->run(m_dt, m_timeout_ms,
                       [&]() { return m_simulator->getStopRequested(); });

    } catch (const Dune::Exception &e) {
      m_simulator->setCurrentErrormessage(e.what());
      SPDLOG_ERROR("{}", m_simulator->errorMessage());
      break;
    }

    c_new = getConcentrations();

    double current_error = computeStoppingCriterion(c_old, c_new);

    if (std::isnan(current_error) or std::isinf(current_error)) {
      m_simulator->setCurrentErrormessage(
          "Simulation failed: NaN  of Inf detected in norm");
      SPDLOG_DEBUG(m_simulator->errorMessage());
      m_simulator->setStopRequested(true);
      break;
    }

    c_old.swap(c_new);

    SPDLOG_CRITICAL("  - time: {}, current error: {}, tolerance: {}, "
                    "steps_below_tolerance: "
                    "{}",
                    tNow, current_error, m_convergence_tolerance,
                    m_steps_below_tolerance);

    if (current_error < m_convergence_tolerance) {
      ++m_steps_below_tolerance;
    } else {
      m_steps_below_tolerance = 0;
    }

    if (m_steps_below_tolerance >= m_steps_to_convergence) {
      m_has_converged.store(true);
      m_simulator->setStopRequested(true);
      SPDLOG_CRITICAL("Simulation has converged");
      break;
    }

    if (m_timeout_ms >= 0.0 &&
        static_cast<double>(timer.elapsed()) >= m_timeout_ms) {
      SPDLOG_DEBUG("Simulation timeout: requesting stop");
      m_simulator->setCurrentErrormessage("Simulation timed out");
      m_simulator->setStopRequested(true);
      break;
    }

    if (m_simulator->getStopRequested()) {
      m_simulator->setCurrentErrormessage("Simulation stopped early");
      SPDLOG_DEBUG("Simulation timeout or stopped early");
      break;
    }

    tNow += m_dt;

    if (m_stopRequested) {
      break;
    }

    recordData(tNow, current_error);
  }
}

void SteadyStateSimulation::run() {
  // since we expect the simulation to either converge or run into timeout,
  // we can set the time to run to infinity for all intents and purposes
  if (m_model.getSimulationSettings().simulatorType == SimulatorType::DUNE) {
    runDune(std::numeric_limits<double>::max());
  } else {
    runPixel(std::numeric_limits<double>::max());
  }
}

void SteadyStateSimulation::requestStop() {
  m_simulator->setStopRequested(true);
  m_stopRequested = true;
}

void SteadyStateSimulation::reset() {
  resetData();
  resetSolver();
}

//////////////////////////////////////////////////////////////////////////////////
// state getters
bool SteadyStateSimulation::hasConverged() const {
  return m_has_converged.load();
}

SteadystateConvergenceMode SteadyStateSimulation::getConvergenceMode() {
  return m_stop_mode;
}

std::size_t SteadyStateSimulation::getStepsBelowTolerance() const {
  return m_steps_below_tolerance;
}

SimulatorType SteadyStateSimulation::getSimulatorType() {
  return m_model.getSimulationSettings().simulatorType;
}

double SteadyStateSimulation::getStopTolerance() const {
  return m_convergence_tolerance;
}

std::vector<double> SteadyStateSimulation::getConcentrations() const {
  std::vector<double> concs;

  for (auto &&idx : m_compartmentIdxs) {
    auto c = m_simulator->getConcentrations(idx);
    concs.insert(concs.end(), c.begin(), c.end());
  }

  return concs;
}

const std::atomic<std::shared_ptr<SteadyStateData>> &
SteadyStateSimulation::getLatestData() const {
  return m_data;
}

std::size_t SteadyStateSimulation::getStepsToConvergence() const {
  return m_steps_to_convergence;
}

double SteadyStateSimulation::getDt() const { return m_dt; }

std::string SteadyStateSimulation::getSolverErrormessage() const {
  return m_simulator->errorMessage();
}

bool SteadyStateSimulation::getSolverStopRequested() const {
  return m_simulator->getStopRequested();
}

double SteadyStateSimulation::getTimeout() const { return m_timeout_ms; }

common::ImageStack SteadyStateSimulation::getConcImage(
    std::size_t timeIndex,
    const std::vector<std::vector<std::size_t>> &speciesToDraw,
    bool normaliseOverAllTimepoints, bool normaliseOverAllSpecies) const {
  // README: adjusted copy from simulate::getConcImage. What to do to reuse this
  // stuff?
  if (m_compartments.empty()) {
    return common::ImageStack{};
  }
  constexpr double minimumNonzeroConc{100.0 *
                                      std::numeric_limits<double>::min()};
  const auto *speciesIndices = &speciesToDraw;
  // default to drawing all species if not specified
  if (speciesToDraw.empty()) {
    speciesIndices = &m_compartmentSpeciesIds; // TODO: fix the type here
  }
  // calculate normalisation for each species
  auto maxConcs{
      data->concentrationMax[nCompletedTimesteps.load(
                                 std::memory_order_seq_cst) -
                             1]}; // TODO: this is not really necessary. just
                                  // get the latest and maintain a running max
  if (!normaliseOverAllTimepoints) {
    // get max for each species at this timepoint
    for (std::size_t ic = 0; ic < m_compartments.size(); ++ic) {
      for (std::size_t is : (*speciesIndices)[ic]) {
        maxConcs[ic][is] = data->avgMinMax[timeIndex][ic][is].max;
      }
    }
  }

  // TODO: extract this into a separate function to make it a bit cleaner
  if (normaliseOverAllSpecies) {
    // normalise over max of all visible species
    double maxC{minimumNonzeroConc};
    for (std::size_t ic = 0; ic < m_compartments.size(); ++ic) {
      for (std::size_t is : (*speciesIndices)[ic]) {
        maxC = std::max(maxC, maxConcs[ic][is]);
      }
    }
    for (auto &c : maxConcs) {
      std::ranges::fill(c, maxC);
    }
  }
  // apply minimum (avoid dividing by zero)
  for (std::size_t ic = 0; ic < m_compartments.size(); ++ic) {
    for (std::size_t is : (*speciesIndices)[ic]) {
      if (maxConcs[ic][is] < minimumNonzeroConc) {
        maxConcs[ic][is] = minimumNonzeroConc;
      }
    }
  }

  // TODO: extract this into separate functions?
  common::ImageStack imgs(imageSize, QImage::Format_ARGB32_Premultiplied);
  imgs.setVoxelSize(m_model.getGeometry().getVoxelSize());
  imgs.fill(0);
  // iterate over compartments
  for (std::size_t ic = 0; ic < m_compartments.size(); ++ic) {
    const auto &voxels{m_compartments[ic]->getVoxels()};
    const auto &conc{data->concentration[timeIndex][ic]};
    std::size_t nSpecies = m_compartmentSpeciesIds[ic].size();
    std::size_t stride{nSpecies + data->concPadding[timeIndex]};
    for (std::size_t ix = 0; ix < voxels.size(); ++ix) {
      int r = 0;
      int g = 0;
      int b = 0;
      for (std::size_t is : (*speciesIndices)[ic]) {
        double c = conc[ix * stride + is] / maxConcs[ic][is];
        const auto &col = m_compartmentSpeciesColors[ic][is];
        r += static_cast<int>(qRed(col) * c);
        g += static_cast<int>(qGreen(col) * c);
        b += static_cast<int>(qBlue(col) * c);
      }
      r = r < 256 ? r : 255;
      g = g < 256 ? g : 255;
      b = b < 256 ? b : 255;
      imgs[voxels[ix].z].setPixel(voxels[ix].p, qRgb(r, g, b));
    }
  }
  return imgs;
}
//////////////////////////////////////////////////////////////////////////////////
// state setters

void SteadyStateSimulation::setConvergenceMode(
    SteadystateConvergenceMode mode) {
  m_stop_mode = mode;
}

void SteadyStateSimulation::setStepsBelowTolerance(
    std::size_t new_numstepssteady) {
  m_steps_below_tolerance = new_numstepssteady;
}

void SteadyStateSimulation::setStopTolerance(double stop_tolerance) {
  m_convergence_tolerance = stop_tolerance;
}

void SteadyStateSimulation::setSimulatorType(SimulatorType type) {
  m_model.getSimulationSettings().simulatorType = type;
  resetData();
  resetSolver();
  initModel();
  selectSimulator();
}

void SteadyStateSimulation::setStepsToConvergence(
    std::size_t steps_to_convergence) {
  m_steps_to_convergence = steps_to_convergence;
}

void SteadyStateSimulation::setDt(double dt) { m_dt = dt; }

void SteadyStateSimulation::setTimeout(double timeout_ms) {
  m_timeout_ms = timeout_ms;
}

} // namespace sme::simulate
