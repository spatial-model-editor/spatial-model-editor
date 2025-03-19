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
      stop_tolerance(stop_tolerance), current_error(1.0) {}

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

void DuneSimSteadyState::setStopTolerance(double stop_tolerance) {
  this->stop_tolerance = stop_tolerance;
}

double
DuneSimSteadyState::computeStoppingCriterion(const std::vector<double> &c_old,
                                             const std::vector<double> &c_new,
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

std::size_t
DuneSimSteadyState::run(double time, double timeout_ms,
                        const std::function<bool()> &stopRunningCallback) {
  if (pDuneImpl2d == nullptr && pDuneImpl3d == nullptr) {
    return 0;
  }
  QElapsedTimer timer;
  timer.start();
  std::size_t steps_within_tolerance = 0;
  std::size_t steps = 0;
  SPDLOG_DEBUG("Starting DuneSimSteadyState::run");
  while (steps_within_tolerance < 10) {
    std::vector<double> c_old = getConcentrations();
    SPDLOG_DEBUG("current time: {}, norm: {}, timeout - elapsed: "
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
      break;
    }
    steps += 1;
    std::vector<double> c_new = getConcentrations();
    current_error = computeStoppingCriterion(c_old, c_new, time);
    if (std::isnan(current_error)) {
      currentErrorMessage = "Simulation failed: NaN detected in norm";
      SPDLOG_DEBUG("Simulation failed: NaN detected");
      break;
    }
    if (current_error < stop_tolerance) {
      ++steps_within_tolerance;
    }
    if (stop_tolerance > 0 and current_error > stop_tolerance) {
      steps_within_tolerance = 0;
    }
    if (stopRunningCallback && stopRunningCallback()) {
      SPDLOG_DEBUG("Simulation cancelled: requesting stop");
      currentErrorMessage = "Simulation cancelled";
      break;
    }
    if (timeout_ms >= 0.0 &&
        static_cast<double>(timer.elapsed()) >= timeout_ms) {
      SPDLOG_DEBUG("Simulation timeout: requesting stop");
      currentErrorMessage = "Simulation timeout";
      break;
    }
  }
  return steps;
}
double DuneSimSteadyState::getCurrentError() const { return current_error; }
double DuneSimSteadyState::getStopTolerance() const { return stop_tolerance; }

} // namespace sme::simulate
