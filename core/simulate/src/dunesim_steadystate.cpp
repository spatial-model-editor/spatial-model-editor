#include "dunesim_steadystate.hpp"
#include "dunesim.hpp"
#include "dunesim_impl.hpp"
#include "sme/duneconverter.hpp"
#include <QElapsedTimer>
#include <QFile>
#include <QImage>
#include <QPainter>
#include <cmath>
#include <vector>

namespace sme::simulate {

// constructor
DuneSimSteadyState::DuneSimSteadyState(
    const model::Model &sbmlDoc, const std::vector<std::string> &compartmentIds,
    double stop_tolerance,
    const std::map<std::string, double, std::less<>> &substitutions)
    : DuneSim(sbmlDoc, compartmentIds, substitutions),
      stop_tolerance(stop_tolerance), current_error(1.0),
      steps_within_tolerance(0), num_steps_steadystate(3) {
  SPDLOG_CRITICAL("DuneSimSteadyState constructor");
}

std::vector<double> DuneSimSteadyState::getConcentrations() const {
  std::vector<double> c;
  if (pDuneImpl2d != nullptr) {
    for (const auto &compartment : pDuneImpl2d->getDuneCompartments()) {
      c.insert(c.end(), compartment.concentration.begin(),
               compartment.concentration.end());
    }
  } else {
    for (const auto &compartment : pDuneImpl3d->getDuneCompartments()) {
      c.insert(c.end(), compartment.concentration.begin(),
               compartment.concentration.end());
    }
  }
  return c;
}

double
DuneSimSteadyState::computeStoppingCriterion(const std::vector<double> &c_old,
                                             const std::vector<double> &c_new,
                                             double dt) {
  // TODO: this works because everything is implemented here, but we have
  // an estimate for dcdt already in the system (?), which however I could not
  // get to work correctly within the stopping criterion.
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
  double relative_norm = dcdt_norm / std::max(c_norm, 1e-12);
  return relative_norm;
}

std::size_t
DuneSimSteadyState::run(double time, double timeout_ms,
                        const std::function<bool()> &stopRunningCallback) {
  if (pDuneImpl2d == nullptr && pDuneImpl3d == nullptr) {
    return 0;
  }
  QElapsedTimer timer;
  timer.start();
  SPDLOG_CRITICAL("Starting DuneSimSteadyState::run");
  std::vector<double> c_old = getConcentrations();
  SPDLOG_CRITICAL("current time: {}, norm: {}, timeout - elapsed: "
                  "{},steps_within_tolerance: {}",
                  time, current_error,
                  timeout_ms - static_cast<double>(timer.elapsed()),
                  steps_within_tolerance);

  try {
    if (pDuneImpl2d != nullptr) {
      pDuneImpl2d->run(time);
    } else {
      pDuneImpl3d->run(time);
    }
    currentErrorMessage.clear();
  } catch (const Dune::Exception &e) {
    currentErrorMessage = e.what();
    SPDLOG_ERROR("{}", currentErrorMessage);
  }
  std::vector<double> c_new = getConcentrations();
  current_error = computeStoppingCriterion(c_old, c_new, time);
  if (std::isnan(current_error) or std::isinf(current_error)) {
    currentErrorMessage = "Simulation failed: NaN  of Inf detected in norm";
    SPDLOG_DEBUG(currentErrorMessage);
  }
  if (current_error < stop_tolerance) {
    ++steps_within_tolerance;
  }
  // needs to have n successive steps below tolerance to be considered steady
  if (steps_within_tolerance > 0 and current_error > stop_tolerance) {
    steps_within_tolerance = 0;
  }
  if (steps_within_tolerance >= num_steps_steadystate) {
    SPDLOG_CRITICAL("Reached steady state");
  }

  if (stopRunningCallback && stopRunningCallback()) {
    SPDLOG_DEBUG("Simulation cancelled: requesting stop");
    currentErrorMessage = "Simulation cancelled";
  }
  if (timeout_ms >= 0.0 && static_cast<double>(timer.elapsed()) >= timeout_ms) {
    SPDLOG_DEBUG("Simulation timeout: requesting stop");
    currentErrorMessage = "Simulation timeout";
  }
  return 1;
}
double DuneSimSteadyState::getCurrentError() const { return current_error; }

double DuneSimSteadyState::getStopTolerance() const { return stop_tolerance; }

std::size_t DuneSimSteadyState::getStepsBelowTolerance() const {
  return steps_within_tolerance;
}

void DuneSimSteadyState::setStopTolerance(double stop_tolerance) {
  this->stop_tolerance = stop_tolerance;
}

std::size_t DuneSimSteadyState::getNumStepsSteady() const {
  return num_steps_steadystate;
}

void DuneSimSteadyState::setNumStepsSteady(std::size_t new_numstepssteady) {
  num_steps_steadystate = new_numstepssteady;
}

} // namespace sme::simulate
