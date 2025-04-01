#include "sme/simulate_steadystate.hpp"
#include "dunesim.hpp"
#include "pixelsim.hpp"
#include "sme/duneconverter.hpp"
#include "sme/image_stack.hpp"
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
    : m_has_converged(false), m_stop_requested(false), m_model(model),
      m_convergence_tolerance(tolerance), m_steps_below_tolerance(0),
      m_steps_to_convergence(steps_to_convergence), m_timeout_ms(timeout_ms),
      m_stop_mode(convergence_mode),
      m_error(std::numeric_limits<double>::max()), m_step(0),
      m_compartmentIds(), m_compartmentSpeciesIds(),
      m_compartmentSpeciesColors(), m_dt(dt) {
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
  m_compartmentSpeciesIdxs.clear();
  m_compartmentSpeciesColors.clear();
  m_compartmentIndices.clear();
  m_compartments.clear();

  for (const auto &compartmentId : m_model.getCompartments().getIds()) {
    m_compartmentIndices.push_back(i);
    std::vector<std::string> sIds;
    std::vector<QRgb> cols;
    const geometry::Compartment *comp = nullptr;
    for (const auto &s : m_model.getSpecies().getIds(compartmentId)) {
      if (!m_model.getSpecies().getIsConstant(s)) {
        sIds.push_back(s.toStdString());
        const auto &field = m_model.getSpecies().getField(s);
        cols.push_back(field->getColor());
        comp = field->getCompartment();
      }
    }

    if (!sIds.empty()) {
      auto sIndices = std::vector<std::size_t>(sIds.size());
      std::iota(sIndices.begin(), sIndices.end(), 0);
      m_compartmentIds.push_back(compartmentId.toStdString());
      m_compartmentSpeciesIds.push_back(std::move(sIds));
      m_compartmentSpeciesIdxs.push_back(std::move(sIndices));
      m_compartmentSpeciesColors.push_back(std::move(cols));
      m_compartments.push_back(comp);
    }
    ++i;
  }

  SPDLOG_DEBUG("  init Model done, variable sizes: {}, {}, {}, {}, {}, {}, {}",
               m_compartmentIds.size(), m_compartmentSpeciesIds.size(),
               m_compartmentSpeciesIdxs.size(),
               m_compartmentSpeciesColors.size(), m_compartments.size(),
               m_compartmentIndices.size(), 0);
}

void SteadyStateSimulation::selectSimulator() {
  if (m_model.getSimulationSettings().simulatorType == SimulatorType::DUNE &&
      m_model.getGeometry().getIsMeshValid()) {
    SPDLOG_CRITICAL(" DUNE Simulator selected");
    m_simulator = std::make_unique<DuneSim>(m_model, m_compartmentIds);
  } else {
    SPDLOG_CRITICAL(" Pixel Simulator selected");
    m_simulator = std::make_unique<PixelSim>(m_model, m_compartmentIds,
                                             m_compartmentSpeciesIds);
  }
}

void SteadyStateSimulation::resetModel() {
  m_model.getSimulationData().clear();
  m_model.getSimulationSettings().times.clear();
  initModel();
}

void SteadyStateSimulation::resetSolver() {
  m_simulator = nullptr;
  m_steps_below_tolerance = 0;
  m_has_converged.store(false);
  m_stop_requested.store(false);
  selectSimulator();
}

double SteadyStateSimulation::computeStoppingCriterion(
    const std::vector<double> &c_old, const std::vector<double> &c_new) {
  // TODO: can I get dc/dt from the solvers somewhere?

  double sum_squared_dcdt = 0.0;
  double dcdt_norm = 0.0;
  if (m_stop_mode == SteadystateConvergenceMode::relative) {
    double sum_squared_c = 0.0;

    for (size_t i = 0; i < c_new.size(); ++i) {
      double dcdt = (c_new[i] - c_old[i]) / std::max(m_dt, 1e-12);
      // Sum of squares for L2 norm calculations
      sum_squared_dcdt += dcdt * dcdt;
      sum_squared_c += c_new[i] * c_new[i];
    }

    dcdt_norm = std::sqrt(sum_squared_dcdt);
    double c_norm = std::sqrt(sum_squared_c);
    dcdt_norm = dcdt_norm / std::max(c_norm, 1e-12);

  } else {

    for (size_t i = 0; i < c_new.size(); ++i) {
      double dcdt = (c_new[i] - c_old[i]) / std::max(m_dt, 1e-12);
      // Sum of squares for L2 norm calculations
      sum_squared_dcdt += dcdt * dcdt;
    }

    dcdt_norm = std::sqrt(sum_squared_dcdt);
  }

  return dcdt_norm;
}

//////////////////////////////////////////////////////////////////////////////////
std::vector<std::vector<double>>
SteadyStateSimulation::computeConcentrationNormalisation(
    const std::vector<std::vector<std::size_t>> &speciesToDraw,
    bool normaliseOverAllSpecies) const {
  std::vector<std::vector<double>> maxConcs;
  double absoluteMin = 100.0 * std::numeric_limits<double>::min();
  if (normaliseOverAllSpecies) {
    double absoluteMax = 0;

    // README: is this what 'simulate' intends to do as well?
    for (std::size_t i = 0; i < m_compartments.size(); ++i) {
      maxConcs.emplace_back(speciesToDraw[i].size(), absoluteMin);
      const auto c = m_simulator->getConcentrations(i);
      for (std::size_t is : (speciesToDraw)[i]) {
        absoluteMax = std::max(absoluteMax, *std::ranges::max_element(c));
      }
    }

    for (auto &&c : maxConcs) {
      std::ranges::fill(c,
                        absoluteMax < absoluteMin ? absoluteMin : absoluteMax);
    }
  } else {
    // get max for each species at this timepoint
    for (std::size_t i = 0; i < m_compartments.size(); ++i) {
      double speciesMax = 0;
      const auto c = m_simulator->getConcentrations(i);
      for (std::size_t is : (speciesToDraw)[i]) {
        speciesMax = std::max(speciesMax, c[is]);
      }
      maxConcs.emplace_back(speciesToDraw[i].size(), speciesMax < absoluteMin
                                                         ? absoluteMin
                                                         : speciesMax);
    }
  }
  return maxConcs;
}

void SteadyStateSimulation::recordData(double timestep, double error) {
  m_error.store(error);
  m_step.store(timestep);
}

void SteadyStateSimulation::resetData() {
  m_error.store(std::numeric_limits<double>::max());
  m_step.store(0);
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
  std::size_t steps = 0;
  SPDLOG_DEBUG("  - time: {}, current time: {}, steps_below_tolerance: "
               "{}, stoprequested: {}, timeout: {}",
               time, tNow, m_steps_below_tolerance,
               m_simulator->getStopRequested(), m_timeout_ms);
  while (tNow + time * relativeTolerance < time) {

    steps++;
    // lock/unlock by hand because no sensibly small scoep to use lockguard is
    // available here
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
      SPDLOG_DEBUG("Simulation has converged");
      break;
    }

    if (m_timeout_ms >= 0.0 &&
        static_cast<double>(timer.elapsed()) >= m_timeout_ms) {
      SPDLOG_DEBUG("Simulation timeout: requesting stop");
      m_simulator->setStopRequested(true);
      m_simulator->setCurrentErrormessage("Simulation timed out");
      break;
    }

    if (m_simulator->getStopRequested() || m_stop_requested.load()) {
      m_simulator->setCurrentErrormessage("Simulation stopped early");
      SPDLOG_DEBUG("Simulation timeout or stopped early");
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

    SPDLOG_DEBUG("  - time: {}, current error: {}, tolerance: {}, "
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
      SPDLOG_DEBUG("Simulation has converged");
      break;
    }

    if (m_timeout_ms >= 0.0 &&
        static_cast<double>(timer.elapsed()) >= m_timeout_ms) {
      SPDLOG_DEBUG("Simulation timeout: requesting stop");
      m_simulator->setCurrentErrormessage("Simulation timed out");
      m_simulator->setStopRequested(true);
      break;
    }

    if (m_simulator->getStopRequested() || m_stop_requested.load()) {
      m_simulator->setCurrentErrormessage("Simulation stopped early");
      SPDLOG_DEBUG("Simulation timeout or stopped early");
      break;
    }

    tNow += m_dt;

    recordData(tNow, current_error);
  }
}

void SteadyStateSimulation::run() {
  // since we expect the simulation to either converge or run into timeout,
  // we can set the time to run to infinity for all intents and purposes
  m_simulator->setStopRequested(false);
  m_stop_requested.store(false);
  if (m_model.getSimulationSettings().simulatorType == SimulatorType::DUNE) {
    SPDLOG_CRITICAL("  - runDune");
    runDune(std::numeric_limits<double>::max());
  } else if (m_model.getSimulationSettings().simulatorType ==
             SimulatorType::Pixel) {
    SPDLOG_CRITICAL("  - runPixel");
    runPixel(std::numeric_limits<double>::max());
  } else {
    SPDLOG_ERROR("Unknown simulator type");
  }
}

void SteadyStateSimulation::requestStop() {
  m_simulator->setStopRequested(true);
  m_stop_requested.store(true);
}

void SteadyStateSimulation::reset() {

  if (m_simulator == nullptr) {
    SPDLOG_DEBUG("  no simulator to reset");
    return;
  }

  resetModel();
  resetSolver();
  resetData();
}

//////////////////////////////////////////////////////////////////////////////////
// state getters
const std::atomic<bool> &SteadyStateSimulation::hasConverged() const {
  return m_has_converged;
}

const std::atomic<bool> &SteadyStateSimulation::getStopRequested() const {
  return m_stop_requested;
}

SteadystateConvergenceMode SteadyStateSimulation::getConvergenceMode() const {
  return m_stop_mode;
}

std::size_t SteadyStateSimulation::getStepsBelowTolerance() const {
  return m_steps_below_tolerance;
}

SimulatorType SteadyStateSimulation::getSimulatorType() const {
  return m_model.getSimulationSettings().simulatorType;
}

double SteadyStateSimulation::getStopTolerance() const {
  return m_convergence_tolerance;
}

std::vector<double> SteadyStateSimulation::getConcentrations() const {
  std::vector<double> concs;
  for (auto &&idx : m_compartmentIndices) {
    auto c = m_simulator->getConcentrations(idx);
    concs.insert(concs.end(), c.begin(), c.end());
  }
  return concs;
}

const std::atomic<double> &SteadyStateSimulation::getLatestStep() const {
  return m_step;
}

const std::atomic<double> &SteadyStateSimulation::getLatestError() const {
  return m_error;
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

sme::common::ImageStack SteadyStateSimulation::getConcentrationImage(
    const std::vector<std::vector<std::size_t>> &speciesToDraw,
    bool normaliseOverAllSpecies) {

  auto imageSize = m_model.getGeometry().getImages().volume();

  auto nestedVecToStr = [](const auto &vec) {
    std::string str = "[";
    for (const auto &v : vec) {
      str += std::to_string(v) + ",";
    }
    return str;
  };

  SPDLOG_DEBUG(" getConcentrationImage: compartments: {}, species to draw: {}, "
               "image size: {} x {} x {}",
               m_compartments.size(), speciesToDraw.size(), imageSize.width(),
               imageSize.height(), imageSize.depth());

  sme::common::ImageStack concentrationImageStack(
      imageSize, QImage::Format_ARGB32_Premultiplied);
  concentrationImageStack.setVoxelSize(m_model.getGeometry().getVoxelSize());
  concentrationImageStack.fill(0);

  if (m_compartments.empty()) {
    SPDLOG_DEBUG("  no compartments --> no image");
    return concentrationImageStack;
  }

  auto maxConcs = computeConcentrationNormalisation(m_compartmentSpeciesIdxs,
                                                    normaliseOverAllSpecies);
  // turn the data at all voxels into rgb
  for (std::size_t i = 0; i < m_compartments.size(); ++i) {
    const auto &voxels = m_compartments[i]->getVoxels();
    std::size_t nSpecies = m_compartmentSpeciesIds[i].size();
    // TODO: place this under lock to avoid reading data while it is written to
    // by the runner thread
    const auto concentrations = m_simulator->getConcentrations(i);
    const auto concentrationPadding = m_simulator->getConcentrationPadding();

    for (std::size_t vi = 0; vi < voxels.size(); ++vi) {
      int r = 0;
      int g = 0;
      int b = 0;

      for (std::size_t s : speciesToDraw[i]) {
        double c = concentrations[vi * (nSpecies + concentrationPadding) + s] /
                   maxConcs[i][s];
        const auto &col = m_compartmentSpeciesColors[i][s];
        r += static_cast<int>(qRed(col) * c);
        g += static_cast<int>(qGreen(col) * c);
        b += static_cast<int>(qBlue(col) * c);
      }

      r = r < 256 ? r : 255;
      g = g < 256 ? g : 255;
      b = b < 256 ? b : 255;

      concentrationImageStack[voxels[vi].z].setPixel(voxels[vi].p,
                                                     qRgb(r, g, b));
    }
  }

  return concentrationImageStack;
}

/**
 * @brief Get the Compartment Species Ids object
 *
 * @return std::vector<std::vector<std::size_t>>
 */
[[nodiscard]] std::vector<std::vector<std::size_t>>
SteadyStateSimulation::getCompartmentSpeciesIdxs() const {
  return m_compartmentSpeciesIdxs;
}

/**
 * @brief Get the Compartment Species Colors object
 *
 * @return std::vector<std::vector<QRgb>>
 */
[[nodiscard]] std::vector<std::vector<QRgb>>
SteadyStateSimulation::getCompartmentSpeciesColors() const {
  return m_compartmentSpeciesColors;
}

[[nodiscard]] std::vector<std::vector<std::string>>
SteadyStateSimulation::getCompartmentSpeciesIds() const {
  return m_compartmentSpeciesIds;
}

//////////////////////////////////////////////////////////////////////////////////
// state setters

void SteadyStateSimulation::setConvergenceMode(
    SteadystateConvergenceMode mode) {
  m_stop_mode = mode;
}

void SteadyStateSimulation::setStopTolerance(double stop_tolerance) {
  m_convergence_tolerance = stop_tolerance;
}

void SteadyStateSimulation::setSimulatorType(SimulatorType type) {
  m_model.getSimulationSettings().simulatorType = type;
  reset();
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
