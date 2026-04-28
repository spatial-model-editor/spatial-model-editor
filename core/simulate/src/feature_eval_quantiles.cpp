#include "sme/feature_eval_quantiles.hpp"
#include <algorithm>
#include <cmath>
#include <ranges>
#include <vector>

namespace sme::simulate {

Quantile::Quantile(double q) { _quantile_value = q; }

auto Quantile::set_quantile(double q) -> void { this->_quantile_value = q; }

auto Quantile::get_quantile() -> double { return _quantile_value; }

auto Quantile::add_data(double v) -> void { this->_copied_data.push_back(v); }

auto Quantile::clear() -> void { this->_copied_data.clear(); }

auto Quantile::reserve(std::size_t capacity) -> void {
  this->_copied_data.reserve(capacity);
}

auto Quantile::compute() -> double {
  if (_copied_data.empty()) {
    return 0.0;
  }

  const auto qValue =
      static_cast<double>(_copied_data.size() - 1) * this->_quantile_value;

  // round q to the nearest full value -> corresponds to pandas' quantile
  // 'nearest' interpolation mode
  // https://pandas.pydata.org/pandas-docs/stable/reference/api/pandas.DataFrame.quantile.html
  const auto q = static_cast<std::size_t>(std::round(qValue));

  std::ranges::nth_element(_copied_data, _copied_data.begin() + q);
  return *(_copied_data.begin() + q);
}
} // namespace sme::simulate
