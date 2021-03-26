#include "simulate_options.hpp"

namespace sme::simulate {

bool operator==(const AvgMinMax &lhs, const AvgMinMax &rhs) {
  return (lhs.avg == rhs.avg) && (lhs.min == rhs.min) && (lhs.max == rhs.max);
}

} // namespace sme::simulate
