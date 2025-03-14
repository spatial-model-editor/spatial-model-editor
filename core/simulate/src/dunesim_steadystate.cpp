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

// private member functions
void DuneSimSteadyState::updateOldState() {

  old_state.clear();
  auto insert_here = old_state.begin();
  if (pDuneImpl2d != nullptr) {
    for (const auto &compartment : pDuneImpl2d->getDuneCompartments()) {
      insert_here = std::copy(compartment.concentration.begin(),
                              compartment.concentration.end(), insert_here);
    }
  } else {
    for (const auto &compartment : pDuneImpl3d->getDuneCompartments()) {
      insert_here = std::copy(compartment.concentration.begin(),
                              compartment.concentration.end(), insert_here);
    }
  }
}

// constructor
DuneSimSteadyState::DuneSimSteadyState(
    const model::Model &sbmlDoc, const std::vector<std::string> &compartmentIds,
    double stop_tolerance, double meta_dt,
    const std::map<std::string, double, std::less<>> &substitutions)
    : DuneSim(sbmlDoc, compartmentIds, substitutions),
      stop_tolerance(stop_tolerance), meta_dt(meta_dt) {
  std::size_t statesize = 0;

  if (pDuneImpl2d != nullptr) {
    for (const auto &compartment : pDuneImpl2d->getDuneCompartments()) {
      statesize += compartment.concentration.size();
    }
  } else {
    for (const auto &compartment : pDuneImpl3d->getDuneCompartments()) {
      statesize += compartment.concentration.size();
    }
  }
  old_state.reserve(statesize);
  updateOldState();
  dcdt.reserve(statesize);
}

// public member functions
std::vector<double> DuneSimSteadyState::getDcdt() const { return dcdt; }

void DuneSimSteadyState::compute_spatial_dcdt() {
  dcdt.clear();
  auto insert_here = dcdt.begin();
  auto current_oldstate = old_state.begin();

  if (pDuneImpl2d != nullptr) {
    for (const auto &compartment : pDuneImpl2d->getDuneCompartments()) {
      std::transform(
          compartment.concentration.begin(), compartment.concentration.end(),
          current_oldstate, insert_here,
          [this](double c, double c_old) { return (c - c_old) / meta_dt; });
      current_oldstate += compartment.concentration.size();
      insert_here += compartment.concentration.size();
    }
  } else {
    for (const auto &compartment : pDuneImpl3d->getDuneCompartments()) {
      std::transform(
          compartment.concentration.begin(), compartment.concentration.end(),
          current_oldstate, insert_here,
          [this](double c, double c_old) { return (c - c_old) / meta_dt; });
      current_oldstate += compartment.concentration.size();
      insert_here += compartment.concentration.size();
    }
  }
}
std::size_t
DuneSimSteadyState::run(double timeout_ms,
                        const std::function<bool()> &stopRunningCallback) {
  if (pDuneImpl2d == nullptr && pDuneImpl3d == nullptr) {
    return 0;
  }
  QElapsedTimer timer;
  timer.start();
  std::size_t steps = 0;
  double current_max_dcdt = 1e3;

  while (std::abs(current_max_dcdt) > stop_tolerance) {

    try {
      if (pDuneImpl2d != nullptr) {
        pDuneImpl2d->run(meta_dt);
      } else {
        pDuneImpl3d->run(meta_dt);
      }
      currentErrorMessage.clear();
    } catch (const Dune::Exception &e) {
      currentErrorMessage = e.what();
      SPDLOG_ERROR("{}", currentErrorMessage);
      return 0;
    }
    calculateDcdt();
    current_max_dcdt = *std::ranges::max_element(
        dcdt, [](double a, double b) { return std::abs(a) < std::abs(b); });
    updateOldState();
    ++steps;

    if (stopRunningCallback && stopRunningCallback()) {
      SPDLOG_DEBUG("Simulation cancelled: requesting stop");
      currentErrorMessage = "Simulation cancelled";
    }
    if (timeout_ms >= 0.0 &&
        static_cast<double>(timer.elapsed()) >= timeout_ms) {
      SPDLOG_DEBUG("Simulation timeout: requesting stop");
      currentErrorMessage = "Simulation timeout";
    }
  }
  return steps;
}

} // namespace sme::simulate
