#pragma once
#include "pixelsim.hpp"
#include "steadystate_helper.hpp"
namespace sme {

namespace simulate {

class PixelSimSteadyState final : public PixelSim, public SteadyStateHelper {
  double stop_tolerance;

public:
  [[nodiscard]] double get_first_order_dcdt_change_max(
      const std::vector<double> &old,
      const std::vector<double> &current) const override;
  [[nodiscard]] std::vector<double> getDcdt() const override;

  PixelSimSteadyState(
      const model::Model &sbmlDoc,
      const std::vector<std::string> &compartmentIds,
      const std::vector<std::vector<std::string>> &compartmentSpeciesIds,
      const std::map<std::string, double, std::less<>> &substitutions,
      double stop_tolerance);

  std::size_t run(double time, double timeout_ms,
                  const std::function<bool()> &stopRunningCallback) override;
};

} // namespace simulate
} // namespace sme
