#pragma once
#include "dunesim.hpp"

namespace sme {
namespace simulate {
class DuneSimSteadyState final : public DuneSim {
  double stop_tolerance;

public:
  explicit DuneSimSteadyState(
      const model::Model &sbmlDoc,
      const std::vector<std::string> &compartmentIds, double stop_tolerance,
      const std::map<std::string, double, std::less<>> &substitutions = {});
  [[nodiscard]] double
  get_first_order_dcdt_change_max(const std::vector<double> &old,
                                  const std::vector<double> &current) const;
  [[nodiscard]] std::vector<double> getDcdt() const;
  std::size_t run(double time, double timeout_ms,
                  const std::function<bool()> &stopRunningCallback) override;
};

} // namespace simulate
} // namespace sme
