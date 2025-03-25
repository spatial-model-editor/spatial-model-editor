#include "simulate_steadystate.hpp"
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
      m_stop_mode(convergence_mode), m_steps(), m_errors(), m_compartmentIdxs(),
      m_compartmentIds(), m_compartmentSpeciesIds(),
      m_compartmentSpeciesColors(), m_dt(dt) {
  m_model.getSimulationSettings().simulatorType = type;
  initModel();
  selectSimulator();
}

//////////////////////////////////////////////////////////////////////////////////

// helpers for solvers
void SteadyStateSimulation::initModel() {
  int i = 0;
  m_compartmentIds.clear();
  m_compartmentSpeciesIds.clear();
  m_compartmentSpeciesColors.clear();
  m_compartmentIdxs.clear();

  for (const auto &compartmentId : m_model.getCompartments().getIds()) {
    m_compartmentIdxs.push_back(i);
    m_compartmentIds.push_back(compartmentId.toStdString());
    m_compartmentSpeciesIds.push_back({});
    m_compartmentSpeciesColors.push_back({});
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

void SteadyStateSimulation::reset_solver() {
  m_simulator = nullptr;
  m_steps_below_tolerance = 0;
  m_has_converged = false;
  initModel();
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

// helpers for data
void SteadyStateSimulation::append_data(double timestep, double error) {
  m_steps.push_back(timestep);
  m_errors.push_back(error);
}

void SteadyStateSimulation::reset_data() {
  m_steps.clear();
  m_errors.clear();
}

//////////////////////////////////////////////////////////////////////////////////
// functions to run stuff

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
      m_has_converged = true;
      m_simulator->setStopRequested(true);
      SPDLOG_DEBUG("Simulation has converged");
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

    tNow += m_dt;

    m_steps.push_back(tNow);

    m_errors.push_back(current_error);
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
      m_has_converged = true;
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

    if (m_simulator->getStopRequested()) {
      m_simulator->setCurrentErrormessage("Simulation stopped early");
      SPDLOG_DEBUG("Simulation timeout or stopped early");
      break;
    }

    tNow += m_dt;

    m_errors.push_back(current_error);

    m_steps.push_back(tNow);
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

//////////////////////////////////////////////////////////////////////////////////
// state getters
bool SteadyStateSimulation::hasConverged() const { return m_has_converged; }

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

double SteadyStateSimulation::getCurrentError() const {
  if (m_errors.size() == 0) {
    SPDLOG_DEBUG("Vector storing errors is empty");
    return std::numeric_limits<double>::max();
  }
  return m_errors.back();
}

double SteadyStateSimulation::getCurrentStep() const {
  if (m_steps.size() == 0) {
    SPDLOG_DEBUG("Vector storing steps is empty");
    return 0;
  }

  return m_steps.back();
}

const std::vector<double> &SteadyStateSimulation::getSteps() const {
  return m_steps;
}

const std::vector<double> &SteadyStateSimulation::getErrors() const {
  return m_errors;
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

//////////////////////////////////////////////////////////////////////////////////
// state setters

void SteadyStateSimulation::setStopMode(SteadystateConvergenceMode mode) {
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
  reset_data();
  reset_solver();
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
