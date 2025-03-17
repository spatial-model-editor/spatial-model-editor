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
    double meta_dt, double stop_tolerance,
    const std::map<std::string, double, std::less<>> &substitutions)
    : PixelSim(sbmlDoc, compartmentIds, compartmentSpeciesIds, substitutions),
      meta_dt(meta_dt), stop_tolerance(stop_tolerance) {

  // awkward but functional way to fix size of vectors
  compute_spatial_dcdt();
  old_state.reserve(dcdt.size());
}

std::size_t
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
  double time = 0;
  std::size_t steps = 0;
  discardedSteps = 0;

  // make this big to avoid immediate stop
  double current_max_dcdt = 1e3;

  while (std::abs(current_max_dcdt) > stop_tolerance) {

    run_step(meta_dt, tNow);
    if (!currentErrorMessage.empty()) {
      return steps;
    }
    ++steps;
    time = tNow + meta_dt; // increment time to run further.

    compute_spatial_dcdt();
    current_max_dcdt = *std::ranges::max_element(
        dcdt, [](double a, double b) { return std::abs(a) < std::abs(b); });

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

void PixelSimSteadyState::compute_spatial_dcdt() {
  dcdt.clear();

  // calculate dcd/dt in all compartments
  // this is a repetition from calculateDcdt() but without spatial averaging
  for (auto &sim : simCompartments) {
    if (useTBB) {
      sim->evaluateReactionsAndDiffusion_tbb();
    } else {
      sim->evaluateReactionsAndDiffusion();
    }
    dcdt.insert(dcdt.end(), sim->getDcdt().begin(), sim->getDcdt().end());
  }

  // membrane contribution to dc/dt
  for (auto &sim : simMembranes) {
    sim->evaluateReactions();
  }

  for (auto &sim : simCompartments) {
    dcdt.insert(dcdt.end(), sim->getDcdt().begin(), sim->getDcdt().end());
  }
}

std::vector<double> PixelSimSteadyState::getDcdt() const { return dcdt; }

const std::vector<double> &PixelSimSteadyState::getOldState() const {
  return old_state;
}
double PixelSimSteadyState::getMetaDt() const { return meta_dt; }
double PixelSimSteadyState::getStopTolerance() const { return stop_tolerance; }
} // namespace sme::simulate
