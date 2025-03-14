#include "dunesim_steadystate.hpp"
#include "dunesim.hpp"
#include "dunesim_impl.hpp"
#include "sme/duneconverter.hpp"
#include <QElapsedTimer>
#include <QFile>
#include <QImage>
#include <QPainter>
#include <vector>

namespace sme::simulate {
DuneSimSteadyState::DuneSimSteadyState(
    const model::Model &sbmlDoc, const std::vector<std::string> &compartmentIds,
    double stop_tolerance,
    const std::map<std::string, double, std::less<>> &substitutions)
    : DuneSim(sbmlDoc, compartmentIds, substitutions),
      stop_tolerance(stop_tolerance) {}

double DuneSimSteadyState::get_dcdt_change_max(
    const std::vector<double> &old, const std::vector<double> &current) const {
  return 0;
}

std::vector<double> DuneSimSteadyState::getDcdt() const { return {}; }

std::size_t
DuneSimSteadyState::run(double timeout_ms,
                        const std::function<bool()> &stopRunningCallback) {
  if (pDuneImpl2d == nullptr && pDuneImpl3d == nullptr) {
    return 0;
  }
  QElapsedTimer timer;
  timer.start();
  double time = 1.0;
  std::size_t steps = 0;
  // primitive way to turn this into a steady state simulation
  // that stops when the maximum change in dcdt is less than stop_tolerance:

  auto old_dcdt = getDcdt();
  auto new_dcdt = getDcdt();
  // make this big to avoid immediate stop
  std::ranges::for_each(new_dcdt, [](double x) { return x + 1e2; });
  auto current_dcdt_change = get_dcdt_change_max(new_dcdt, old_dcdt);

  while (current_dcdt_change > stop_tolerance) {
    old_dcdt = getDcdt();

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
      return 0;
    }
    ++steps;
    new_dcdt = getDcdt();
    current_dcdt_change = get_dcdt_change_max(new_dcdt, old_dcdt);

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
