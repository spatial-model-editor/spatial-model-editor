#include "pixelsim_steadystate.hpp"
#include "pixelsim_impl.hpp"
#include <QElapsedTimer>
#include <cmath>
#include <oneapi/tbb/global_control.h>
#include <oneapi/tbb/info.h>

namespace sme::simulate {

PixelSimSteadyState::PixelSimSteadyState(
    const model::Model &sbmlDoc, const std::vector<std::string> &compartmentIds,
    const std::vector<std::vector<std::string>> &compartmentSpeciesIds,
    double stop_tolerance,
    const std::map<std::string, double, std::less<>> &substitutions)
    : PixelSim(sbmlDoc, compartmentIds, compartmentSpeciesIds, substitutions),
      stop_tolerance(stop_tolerance) {}

double
PixelSimSteadyState::run(double timeout_ms,
                         const std::function<bool()> &stopRunningCallback) {
  SPDLOG_TRACE("  - max rel local err {}", errMax.rel);
  SPDLOG_TRACE("  - max abs local err {}", errMax.abs);
  SPDLOG_TRACE("  - max stepsize {}", maxTimestep);
  currentErrorMessage.clear();
  oneapi::tbb::global_control control(
      oneapi::tbb::global_control::max_allowed_parallelism, numMaxThreads);
  QElapsedTimer timer;
  timer.start();
  double tNow = 0;
  double time = 1e6;
  std::size_t steps = 0;
  std::size_t steps_within_tolerance = 0;
  discardedSteps = 0;
  // do timesteps until we reach t
  constexpr double relativeTolerance = 1e-12;
  double norm = 1.0;
  while (steps_within_tolerance < 10) {
    if (steps % 10000 == 0) {
      SPDLOG_DEBUG("current time: {}, norm: {}, timeout - elapsed: "
                   "{},steps_within_tolerance: {}",
                   tNow + time * relativeTolerance, norm,
                   timeout_ms - static_cast<double>(timer.elapsed()),
                   steps_within_tolerance);
    }

    double t_old = tNow;

    std::vector<double> c_ = getConcentrations();

    tNow = run_step(time, tNow);

    double t_new = tNow;

    std::vector<double> c = getConcentrations();

    norm = compute_stopping_criterion(c_, c, t_new - t_old);

    if (!currentErrorMessage.empty()) {
      return norm;
    }
    ++steps;

    if (norm < stop_tolerance) {
      steps_within_tolerance += 1;
    }
    if (steps_within_tolerance > 0 and norm > stop_tolerance) {
      steps_within_tolerance = 0;
    }
    if (std::isnan(norm)) {
      currentErrorMessage = "Simulation failed: NaN detected in norm";
      SPDLOG_DEBUG("Simulation failed: NaN detected");
      return norm;
    }
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
      return norm;
    }
  }
  SPDLOG_DEBUG("t={} integrated using {} steps ({:3.1f}% discarded)", time,
               steps + discardedSteps,
               static_cast<double>(100 * discardedSteps) /
                   static_cast<double>(steps + discardedSteps));
  SPDLOG_DEBUG("final ||dcdt||: {}, tolerance: {}, steps within tolerance {}",
               norm, stop_tolerance, steps_within_tolerance);
  return norm;
}

std::vector<double> PixelSimSteadyState::getConcentrations() const {
  std::vector<double> c;
  for (auto &&sim : simCompartments) {
    auto c_sim = sim->getConcentrations();
    c.insert(c.end(), c_sim.begin(), c_sim.end());
  }
  return c;
}

double PixelSimSteadyState::compute_stopping_criterion(
    const std::vector<double> &c_old, const std::vector<double> &c_new,
    double dt) {
  // TODO: this works because everything is implemented here, but we have
  // an estimate for dcdt already in the system, which however I could not
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

double PixelSimSteadyState::getStopTolerance() const { return stop_tolerance; }
} // namespace sme::simulate
