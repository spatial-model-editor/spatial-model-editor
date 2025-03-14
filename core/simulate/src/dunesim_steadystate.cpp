#include "dunesim_steadystate.hpp"
#include <vector>

namespace sme::simulate {
DuneSimSteadyState::DuneSimSteadyState(
    const model::Model &sbmlDoc, const std::vector<std::string> &compartmentIds,
    double stop_tolerance,
    const std::map<std::string, double, std::less<>> &substitutions)
    : DuneSim(sbmlDoc, compartmentIds, substitutions),
      stop_tolerance(stop_tolerance) {}

double DuneSimSteadyState::get_first_order_dcdt_change_max(
    const std::vector<double> &old, const std::vector<double> &current) const {
  return 0;
}

std::vector<double> DuneSimSteadyState::getDcdt() const { return {}; }

std::size_t
DuneSimSteadyState::run(double time, double timeout_ms,
                        const std::function<bool()> &stopRunningCallback) {
  return 0;
}

} // namespace sme::simulate
