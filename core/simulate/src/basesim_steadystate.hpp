// Simulator interface class

#pragma once

#include <vector>

namespace sme::simulate {

class BaseSimSteadyState {
public:
  virtual double
  get_first_order_dcdt_change_max(const std::vector<double> &old,
                                  const std::vector<double> &current) const = 0;
  virtual std::vector<double> getDcdt() const = 0;
};

} // namespace sme::simulate
