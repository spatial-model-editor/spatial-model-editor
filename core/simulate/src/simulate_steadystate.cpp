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

void SteadyStateSimulation::selectSimulator() {
  if (m_model.getSimulationSettings().simulatorType == SimulatorType::DUNE)
    m_simulator =
        std::make_unique<DuneSim>(m_model, /*TODO: Add compartment ids here*/);
  else {
    m_simulator = std::make_unique<PixelSim>(
        m_model, /*TODO: Add compartment ids here and other shit*/);
  }
}

void SteadyStateSimulation::runPixel(double time) {

  m_simulator->setCurrentErrormessage("");
  oneapi::tbb::global_control control(
      oneapi::tbb::global_control::max_allowed_parallelism,
      m_model.getSimulationSettings().options.pixel.maxThreads);
  QElapsedTimer timer;
  timer.start();
  double tNow = 0;
  // do timesteps until we reach t
  constexpr double relativeTolerance = 1e-12;
  while (tNow + time * relativeTolerance < time) {
    auto c_before = getConcentrations();
    double tBefore = time;
    tNow = m_simulator->run_step(time, tNow);

    auto c_after = getConcentrations();
    auto current_error =
        computeStoppingCriterion(c_before, c_after, tNow - tBefore);
    if (std::isnan(current_error) or std::isinf(current_error)) {
      m_simulator->setCurrentErrormessage(
          "Simulation failed: NaN  of Inf detected in norm");
      SPDLOG_DEBUG(currentErrorMessage);
    }
    m_steps.push_back(tNow);
    m_errors.push_back(current_error);

    if (current_error < m_convergenceTolerance) {
      m_steps_below_tolerance++;
    } else {
      m_steps_below_tolerance = 0;
    }

    if (m_steps_below_tolerance >= m_steps_to_convergence) {
      m_has_converged.store(true);
      m_simulator->setStopRequested(true);
      SPDLOG_DEBUG("Simulation has converged");
    }

    if (m_simulator->getCurrentErrorMessage().empty()) {
      break;
    }

    if (m_steps_below_tolerance >= m_steps_to_convergence) {
      m_has_converged.store(true);
    }

    if (m_timeout_ms >= 0.0 &&
        static_cast<double>(timer.elapsed()) >= m_timeout_ms) {
      SPDLOG_DEBUG("Simulation timeout: requesting stop");
      m_simulator->setStopRequested(true);
      break;
    }
    if (m_simulator->getStopRequested().load()) {
      m_simulator->setCurrentErrormessage("Simulation stopped early");
      SPDLOG_DEBUG("Simulation timeout or stopped early");
      break;
    }
  }
  SPDLOG_DEBUG("t={} integrated using {} steps ({:3.1f}% discarded)", time,
               steps + discardedSteps,
               static_cast<double>(100 * discardedSteps) /
                   static_cast<double>(steps + discardedSteps));
}

void SteadyStateSimulation::runDune(double time) {

  // FIXME: do some better shit here to make this work!
  QElapsedTimer timer;
  timer.start();
  double tNow = 0.;
  double relativeTolerance = 1e-12;
  while (tNow + time * relativeTolerance < time) {

    SPDLOG_CRITICAL("Starting DuneSimSteadyState::run");
    std::vector<double> c_old = getConcentrations();
    SPDLOG_DEBUG(
        " DuneSimSteadyState current time: {}, norm: {}, timeout - elapsed: "
        "{},steps_within_tolerance: {}, num_steps_steadystate: {}",
        time, current_error, timeout_ms - static_cast<double>(timer.elapsed()),
        steps_within_tolerance, num_steps_steadystate);

    try {
      tNow = m_simulator->run_step(time, tNow);

    } catch (const Dune::Exception &e) {
      m_simulator->setCurrentErrormessage(e.what());
      SPDLOG_ERROR("{}", currentErrorMessage);
      break;
    }

    std::vector<double> c_new = getConcentrations();

    double current_error = computeStoppingCriterion(c_old, c_new, time);

    if (std::isnan(current_error) or std::isinf(current_error)) {
      m_simulator->setCurrentErrormessage(
          "Simulation failed: NaN  of Inf detected in norm");
      SPDLOG_DEBUG(currentErrorMessage);
      break;
    }
    m_errors.push_back(current_error);
    m_steps.push_back(tNow);

    if (current_error < m_convergenceTolerance) {
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
      m_simulator->setCurrentErrormessage("Simulation timeout");
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
bool SteadyStateSimulation::hasConverged() const {
  return m_has_converged.load();
}

SteadystateStopMode SteadyStateSimulation::getStopMode() { return m_stop_mode; }

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

// state setters

void SteadyStateSimulation::setStopMode(SteadystateStopMode mode) {
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
  if (m_stop_mode == SteadystateStopMode::relative) {
    dcdt_norm = dcdt_norm / std::max(c_norm, 1e-12);
  }

  return dcdt_norm;
}

} // namespace sme::simulate
