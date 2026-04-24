#include <cmath>
#include <vector>

namespace sme::simulate {

class Quantile {
  double _quantile_value;
  std::vector<double> _copied_data;

public:
  Quantile(double q, std::size_t capacity = 0);
  auto operator()(auto &&data) -> double;
};

} // namespace sme::simulate