#include <cmath>
#include <vector>

namespace sme::simulate {

/**
@brief TODO
 */
class Quantile {
  double _quantile_value;
  std::vector<double> _copied_data;

public:

  auto set_quantile(double q) -> void {
    this->_quantile_value = q;
  }

  auto get_quantile() ->double {
    return _quantile_value;
  }

  /**
  @brief TODO
  */
  Quantile(double q);

  /**
  @brief TODO
  */
  auto operator()(const std::vector<double>& data, const std::vector<std::size_t> &voxelRegions,
                      std::size_t targetRegion) -> double;
};

} // namespace sme::simulate