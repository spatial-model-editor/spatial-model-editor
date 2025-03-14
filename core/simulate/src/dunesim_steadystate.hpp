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

  std::vector<double> getDcdt() const override;

  std::size_t run(double timeout_ms,
                  const std::function<bool()> &stopRunningCallback) override;
};

} // namespace simulate
} // namespace sme
