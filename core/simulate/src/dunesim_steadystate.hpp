#pragma once
#include "dunesim.hpp"
#include "steadystate_helper.hpp"
namespace sme {
namespace simulate {
class DuneSimSteadyState final : public DuneSim, public SteadyStateHelper {
  double stop_tolerance;

public:
  explicit DuneSimSteadyState(
      const model::Model &sbmlDoc,
      const std::vector<std::string> &compartmentIds, double stop_tolerance,
      const std::map<std::string, double, std::less<>> &substitutions = {});

  double get_first_order_dcdt_change_max(
      const std::vector<double> &old,
      const std::vector<double> &current) const override;

  std::vector<double> getDcdt() const override;

  std::size_t run(double time, double timeout_ms,
                  const std::function<bool()> &stopRunningCallback) override;
};

} // namespace simulate
} // namespace sme
