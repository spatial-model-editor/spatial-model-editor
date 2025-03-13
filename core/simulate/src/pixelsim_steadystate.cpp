#include "pixelsim_steadystate.hpp"
#include "pixelsim_impl.hpp"
#include <QElapsedTimer>
#include <algorithm>
#include <cmath>
#include <oneapi/tbb/global_control.h>
#include <oneapi/tbb/info.h>

namespace sme::simulate {

PixelSimSteadyState::PixelSimSteadyState(
    const model::Model &sbmlDoc, const std::vector<std::string> &compartmentIds,
    const std::vector<std::vector<std::string>> &compartmentSpeciesIds,
    const std::map<std::string, double, std::less<>> &substitutions,
    double stop_tolerance)
    : PixelSim(sbmlDoc, compartmentIds, compartmentSpeciesIds, substitutions),
      stop_tolerance(stop_tolerance) {}

double PixelSimSteadyState::get_first_order_dcdt_change_max(
    const std::vector<double> &old, const std::vector<double> &current) const {
  double max = 0;
  for (std::size_t i = 0; i < old.size(); i++) {
    double diff = std::abs(old[i] - current[i]);
    if (diff > max) {
      max = diff;
    }
  }
  return max;
}

std::size_t
PixelSimSteadyState::run(double time, double timeout_ms,
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
  std::size_t steps = 0;
  discardedSteps = 0;
  // do timesteps until we reach t
  constexpr double relativeTolerance = 1e-12;

  std::vector<double> old_Dcdt = getDcdt();
  std::vector<double> new_Dcdt = getDcdt();

  // make this big to avoid immediate stop
  std::ranges::for_each(new_Dcdt, [](double x) { return x + 1e3; });

  double current_max_dcdt = get_first_order_dcdt_change_max(new_Dcdt, old_Dcdt);

  while (current_max_dcdt > stop_tolerance) {
    calculateDcdt();
    old_Dcdt = getDcdt();
    run_step(time, tNow);
    if (!currentErrorMessage.empty()) {
      return steps;
    }
    ++steps;
    calculateDcdt();
    new_Dcdt = getDcdt();
    current_max_dcdt = get_first_order_dcdt_change_max(new_Dcdt, old_Dcdt);

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
      return steps;
    }
  }
  SPDLOG_DEBUG("t={} integrated using {} steps ({:3.1f}% discarded)", time,
               steps + discardedSteps,
               static_cast<double>(100 * discardedSteps) /
                   static_cast<double>(steps + discardedSteps));
  return steps;
}

std::vector<double> PixelSimSteadyState::getDcdt() const {

  // TODO: check if this is a problem performancewise because of the allocation
  // involved here. Maybe use a side effect instead or find a good way to
  // preallocate at least
  std::vector<double> dcdt;

  // is this correct or is there a better way to do it?
  for (auto &&compartment : simCompartments) {
    auto dcdtCompartment = compartment->getDcdt();
    dcdt.insert(dcdt.end(), dcdtCompartment.begin(), dcdtCompartment.end());
  }
  dcdt.shrink_to_fit();

  return dcdt;
}
} // namespace sme::simulate
