#pragma once
#include "dunesim.hpp"
#include "dunesim_impl.hpp"
#include "sme/duneconverter.hpp"
#include "steadystate_helper.hpp"
namespace sme {
namespace simulate {
class DuneSimSteadyState final : public DuneSim, public SteadyStateHelper {
  double stop_tolerance;
  std::vector<double> old_state;
  std::vector<double> dcdt;
  double meta_dt;

  void updateOldState();
  void calculateDcdt();

public:
  explicit DuneSimSteadyState(
      const model::Model &sbmlDoc,
      const std::vector<std::string> &compartmentIds, double stop_tolerance,
      double meta_dt = 1.0,
      const std::map<std::string, double, std::less<>> &substitutions = {});

  std::vector<double> getDcdt() const override;

  std::size_t run(double timeout_ms,
                  const std::function<bool()> &stopRunningCallback) override;
};

} // namespace simulate
} // namespace sme
