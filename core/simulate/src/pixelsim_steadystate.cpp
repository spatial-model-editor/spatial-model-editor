#include "pixelsim_steadystate.hpp"
#include "pixelsim_impl.hpp"
#include <QElapsedTimer>
#include <cmath>
#include <oneapi/tbb/global_control.h>
#include <oneapi/tbb/info.h>
#include <spdlog/spdlog.h>

namespace sme::simulate {

PixelSimSteadyState::PixelSimSteadyState(
    const model::Model &sbmlDoc, const std::vector<std::string> &compartmentIds,
    const std::vector<std::vector<std::string>> &compartmentSpeciesIds,
    double stop_tolerance,
    const std::map<std::string, double, std::less<>> &substitutions)
    : PixelSim(sbmlDoc, compartmentIds, compartmentSpeciesIds, substitutions),
      stop_tolerance(stop_tolerance), current_error(0),
      steps_within_tolerance(0), num_steps_steadystate(3) {}

std::size_t
PixelSimSteadyState::run(double time, double timeout_ms,
                         const std::function<bool()> &stopRunningCallback) {
  SPDLOG_TRACE("  - max rel local err {}", errMax.rel);
  SPDLOG_TRACE("  - max abs local err {}", errMax.abs);
  SPDLOG_TRACE("  - max stepsize {}", maxTimestep);
  SPDLOG_CRITICAL("Starting PixelSimSteadyState::run: time: {}, timeout: {}",
                  time, timeout_ms);
  currentErrorMessage.clear();
  oneapi::tbb::global_control control(
      oneapi::tbb::global_control::max_allowed_parallelism, numMaxThreads);
  QElapsedTimer timer;
  timer.start();
  double tNow = 0;
  std::size_t steps = 0;
  discardedSteps = 0;
  // do timesteps until we reach t
  constexpr double relativeTolerance = 1e-12;
  current_error = 1.0;

  std::vector<double> c_ = getConcentrations();
  while (tNow + time * relativeTolerance < time) {

    tNow = run_step(time, tNow);

    if (!currentErrorMessage.empty()) {
      break;
    }
    ++steps;
    if (timeout_ms >= 0.0 &&
        static_cast<double>(timer.elapsed()) >= timeout_ms) {
      SPDLOG_DEBUG("Simulation timeout: requesting stop");
      setStopRequested(true);
    }
    if (stopRunningCallback && stopRunningCallback()) {
      setStopRequested(true);
      SPDLOG_DEBUG("Simulation cancelled: requesting stop");
    }
    if (stopRequested.load()) {
      currentErrorMessage = "Simulation stopped early";
      SPDLOG_DEBUG("Simulation timeout or stopped early");
      break;
    }
    SPDLOG_DEBUG("current time: {}, norm: {}, timeout - elapsed: "
                 "{},steps_within_tolerance: {}",
                 tNow + time * relativeTolerance, current_error,
                 timeout_ms - static_cast<double>(timer.elapsed()),
                 steps_within_tolerance);
  }
  // README: put this here because this is how the dune solver works.
  // Not sure if there is a better way
  std::vector<double> c = getConcentrations();
  current_error = computeStoppingCriterion(c_, c, time);

  if (current_error < stop_tolerance) {
    steps_within_tolerance += 1;
  }

  if (steps_within_tolerance > 0 and current_error > stop_tolerance) {
    steps_within_tolerance = 0;
  }

  if (std::isnan(current_error) or std::isinf(current_error)) {
    currentErrorMessage = "Simulation failed: NaN or Inf detected in norm";
    SPDLOG_DEBUG(currentErrorMessage);
    setStopRequested(true);
  }

  if (hasConverged()) {
    SPDLOG_CRITICAL("Reached steady state");
    setStopRequested(true);
  }

  SPDLOG_DEBUG(
      "PixelSimSteadyState t={} integrated using {} steps ({:3.1f}% discarded)",
      time, steps + discardedSteps,
      static_cast<double>(100 * discardedSteps) /
          static_cast<double>(steps + discardedSteps));
  SPDLOG_CRITICAL(
      "final ||dcdt||: {}, tolerance: {}, steps within tolerance {}",
      current_error, stop_tolerance, steps_within_tolerance);
  return steps;
}

double PixelSimSteadyState::getCurrentError() const { return current_error; }

std::vector<double> PixelSimSteadyState::getConcentrations() const {
  std::vector<double> c;
  for (auto &&sim : simCompartments) {
    auto c_sim = sim->getConcentrations();
    c.insert(c.end(), c_sim.begin(), c_sim.end());
  }
  return c;
}

double
PixelSimSteadyState::computeStoppingCriterion(const std::vector<double> &c_old,
                                              const std::vector<double> &c_new,
                                              double dt) {
  // TODO: this works because everything is implemented here, but we have
  // an estimate for dcdt already in the system, which however I could not
  // get to work correctly within the stopping criterion.
  // is it the spatial average that messes it up?
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

double PixelSimSteadyState::getStopTolerance() const { return stop_tolerance; }

void PixelSimSteadyState::setStopTolerance(double stop_tolerance) {
  this->stop_tolerance = stop_tolerance;
}

std::size_t PixelSimSteadyState::getStepsBelowTolerance() const {
  return steps_within_tolerance;
}

std::size_t PixelSimSteadyState::getNumStepsSteady() const {
  return num_steps_steadystate;
}

void PixelSimSteadyState::setNumStepsSteady(std::size_t new_numstepssteady) {
  num_steps_steadystate = new_numstepssteady;
}

bool PixelSimSteadyState::hasConverged() const {
  return steps_within_tolerance >= num_steps_steadystate;
}

} // namespace sme::simulate
