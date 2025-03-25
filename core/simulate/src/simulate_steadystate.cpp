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
#include <memory>
#include <oneapi/tbb/global_control.h>
#include <oneapi/tbb/info.h>
#include <vector>

namespace sme::simulate {

// lifecycle
SteadyStateSimulation::SteadyStateSimulation(
    sme::model::Model &model, SimulatorType type, double tolerance,
    std::size_t steps_to_convergence,
    SteadystateConvergenceMode convergence_mode, std::size_t timeout_ms,
    double dt)
    : m_has_converged(false), m_model(model), m_convergenceTolerance(tolerance),
      m_steps_below_tolerance(0), m_steps_to_convergence(steps_to_convergence),
      m_timeout_ms(timeout_ms), m_stop_mode(convergence_mode), m_steps(),
      m_errors(), m_compartmentIdxs(), m_compartmentIds(),
      m_compartmentSpeciesIds(), m_compartmentSpeciesColors(), m_dt(dt) {
  initModel();
  selectSimulator();
}

// helpers
void SteadyStateSimulation::initModel() {
  int i = 0;
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

// run stuff

void SteadyStateSimulation::runPixel(double time) {

  m_simulator->setCurrentErrormessage("");

  oneapi::tbb::global_control control(
      oneapi::tbb::global_control::max_allowed_parallelism,
      m_model.getSimulationSettings().options.pixel.maxThreads);

  QElapsedTimer timer;
  double tNow = 0;
  constexpr double relativeTolerance = 1e-12;

  std::vector<double> c_old = getConcentrations();
  std::vector<double> c_new;
  c_new.reserve(c_old.capacity());

  // do timesteps until we reach t
  timer.start();
  while (tNow + time * relativeTolerance < time) {
    double tBefore = time;

    m_simulator->run(m_dt, m_timeout_ms,
                     [&]() { return m_simulator->getStopRequested(); });

    c_new = getConcentrations();

    auto current_error = computeStoppingCriterion(c_old, c_new, tNow - tBefore);

    c_old.swap(c_new);

    if (std::isnan(current_error) or std::isinf(current_error)) {
      m_simulator->setCurrentErrormessage(
          "Simulation failed: NaN  of Inf detected in norm");
      SPDLOG_DEBUG(currentErrorMessage);
    }

    tNow += m_dt;

    m_steps.push_back(tNow);

    m_errors.push_back(current_error);

    if (current_error < m_convergenceTolerance) {
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

    if (m_timeout_ms >= 0.0 &&
        static_cast<double>(timer.elapsed()) >= m_timeout_ms) {
      SPDLOG_DEBUG("Simulation timeout: requesting stop");
      m_simulator->setStopRequested(true);
      break;
    }

    if (m_simulator->getStopRequested()) {
      m_simulator->setCurrentErrormessage("Simulation stopped early");
      SPDLOG_DEBUG("Simulation timeout or stopped early");
      break;
    }
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

  // use dt here to avoid too long intervals for checking the stopping criterion

  while (tNow + time * relativeTolerance < time) {

    try {
      m_simulator->run(m_dt, m_timeout_ms,
                       [&]() { return m_simulator->getStopRequested(); });

    } catch (const Dune::Exception &e) {
      m_simulator->setCurrentErrormessage(e.what());
      SPDLOG_ERROR("{}", currentErrorMessage);
      break;
    }

    tNow += m_dt;

    c_new = getConcentrations();

    double current_error = computeStoppingCriterion(c_old, c_new, time);

    c_old.swap(c_new);

    m_errors.push_back(current_error);

    m_steps.push_back(tNow);

    if (current_error < m_convergenceTolerance) {
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
      m_simulator->setCurrentErrormessage("Simulation timeout");
      break;
    }

    if (m_simulator->getStopRequested()) {
      m_simulator->setCurrentErrormessage("Simulation stopped early");
      SPDLOG_DEBUG("Simulation timeout or stopped early");
      break;
    }
  }
}

void SteadyStateSimulation::run(double time) {

  if (m_model.getSimulationSettings().simulatorType == SimulatorType::DUNE) {
    runDune(time);
  } else {
    runPixel(time);
  }
}

// state getters
bool SteadyStateSimulation::hasConverged() const { return m_has_converged; }

SteadystateConvergenceMode SteadyStateSimulation::getStopMode() {
  return m_stop_mode;
}

std::size_t SteadyStateSimulation::getStepsBelowTolerance() const {
  return m_steps_below_tolerance;
}

SimulatorType SteadyStateSimulation::getSimulatorType() {
  return m_model.getSimulationSettings().simulatorType;
}

double SteadyStateSimulation::getStopTolerance() const {
  return m_convergenceTolerance;
}

std::vector<double> SteadyStateSimulation::getConcentrations() const {}

double SteadyStateSimulation::getCurrentError() const {
  return m_errors.back();
}

double SteadyStateSimulation::getCurrentStep() const { return m_steps.back(); }

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

// state setters

void SteadyStateSimulation::setStopMode(SteadystateConvergenceMode mode) {
  m_stop_mode = mode;
}

void SteadyStateSimulation::setStepsBelowTolerance(
    std::size_t new_numstepssteady) {
  m_steps_below_tolerance = new_numstepssteady;
}

void SteadyStateSimulation::setStopTolerance(double stop_tolerance) {
  m_convergenceTolerance = stop_tolerance;
}

void SteadyStateSimulation::setSimulatorType(SimulatorType type) {
  m_model.getSimulationSettings().simulatorType = type;
}

void SteadyStateSimulation::setStepsToConvergence(
    std::size_t steps_to_convergence) {
  m_steps_to_convergence = steps_to_convergence;
}

void SteadyStateSimulation::setDt(double dt) { m_dt = dt; }

double SteadyStateSimulation::computeStoppingCriterion(
    const std::vector<double> &c_old, const std::vector<double> &c_new,
    double dt) {
  double sum_squared_dcdt = 0.0;
  double sum_squared_c = 0.0;

  for (size_t i = 0; i < c_new.size(); ++i) {
    double dcdt = (c_new[i] - c_old[i]) / std::max(dt, 1e-12);
    // Sum squares for L2 norm calculations
    sum_squared_dcdt += dcdt * dcdt;
    sum_squared_c += c_new[i] * c_new[i];
  }

  double c_norm = std::sqrt(sum_squared_c);
  double dcdt_norm = std::sqrt(sum_squared_dcdt);
  if (m_stop_mode == SteadystateConvergenceMode::relative) {
    dcdt_norm = dcdt_norm / std::max(c_norm, 1e-12);
  }

  return dcdt_norm;
}

} // namespace sme::simulate
