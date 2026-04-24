#include "sme/feature_eval_quantiles.hpp"
#include <algorithm>
#include <cmath>
#include <vector>

namespace sme::simulate {

Quantile::Quantile(double q, std::size_t capacity) {
  _quantile_value = q;

  if (capacity > 0) {
    this->_copied_data.reserve(capacity);
  }
}

auto Quantile::operator()(auto &&data) -> double {

    _copied_data.resize(data.size());
  std::copy(data.begin(), data.end(), _copied_data.begin());

  // TODO: this needs a policy on how to behave wrt to rounding the value
  // FIXME: the value returned here is the closest to the actual quantile, not the quantile itself. Good enough for structure, but big difference. fix this.
  auto q = static_cast<std::size_t>(
      std::round(static_cast<double>(data.size()) * this->_quantile_value));
  std::nth_element(std::begin(_copied_data), std::begin(_copied_data) + q,
                   std::end(_copied_data));
  auto result = *(std::begin(_copied_data) + q);
  return result;
}
} // namespace sme::simulate
