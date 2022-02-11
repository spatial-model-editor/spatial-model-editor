#include "sme/simulate_data.hpp"

namespace sme::simulate {

void SimulationData::clear() {
  timePoints.clear();
  concentration.clear();
  avgMinMax.clear();
  concentrationMax.clear();
  concPadding.clear();
  xmlModel.clear();
}

std::size_t SimulationData::size() const { return timePoints.size(); }

void SimulationData::reserve(std::size_t n) {
  timePoints.reserve(n);
  concentration.reserve(n);
  avgMinMax.reserve(n);
  concentrationMax.reserve(n);
  concPadding.reserve(n);
}

void SimulationData::pop_back() {
  timePoints.pop_back();
  concentration.pop_back();
  avgMinMax.pop_back();
  concentrationMax.pop_back();
  concPadding.pop_back();
}

} // namespace sme::simulate
