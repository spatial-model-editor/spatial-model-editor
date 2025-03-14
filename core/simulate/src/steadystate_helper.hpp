#pragma once
#include <vector>

namespace sme {
namespace simulate {
class SteadyStateHelper {
public:
  [[nodiscard]] virtual double
  get_first_order_dcdt_change_max(const std::vector<double> &old,
                                  const std::vector<double> &current) const = 0;
  [[nodiscard]] virtual std::vector<double> getDcdt() const = 0;

  virtual ~SteadyStateHelper() = default;
};
} // namespace simulate
} // namespace sme
