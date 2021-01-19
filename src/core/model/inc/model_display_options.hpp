#pragma once

#include <vector>

namespace sme {

namespace model {

struct DisplayOptions {
  std::vector<bool> showSpecies;
  bool showMinMax{true};
  bool normaliseOverAllTimepoints{true};
  bool normaliseOverAllSpecies{true};
};

} // namespace model

} // namespace sme
