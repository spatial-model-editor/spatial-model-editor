#include "sme/feature_eval_quantiles.hpp"
#include <algorithm>
#include <cmath>
#include <ranges>
#include <vector>

namespace sme::simulate {

Quantile::Quantile(double q) { _quantile_value = q; }

auto Quantile::operator()(const std::vector<double>& data,
                          const std::vector<std::size_t> &voxelRegions,
                          std::size_t targetRegion) -> double {

  this->_copied_data.clear();
  // b/c nth element reorders the data, we need to copy it in order to avoid side effects on the original data
  if (data.size() > _copied_data.capacity()) {
    this->_copied_data.reserve(data.size());
  }

  this->_copied_data.resize(data.size(), 0);

  // filter data such that voxelRegions[i] == targetRegion is not in the _copied_data
  for (auto i = 0ull; i < data.size(); ++i) {
    if (voxelRegions[i] == targetRegion) {
        continue;
    }
    else {
        // add the data to the copied data used to compute the quantile
        _copied_data.push_back(data[i]);

    }
  }

  auto q = static_cast<double>(data.size()) * this->_quantile_value;

  // round q to the nearest full value -> corresponds to pandas' quantile
  // 'nearest' interpolation mode
  // https://pandas.pydata.org/pandas-docs/stable/reference/api/pandas.DataFrame.quantile.html
  q = static_cast<std::size_t>(std::round(q));

  std::ranges::nth_element(_copied_data, _copied_data.begin() + q);
  auto result = *(_copied_data.begin() + q);
  return result;
}
} // namespace sme::simulate
