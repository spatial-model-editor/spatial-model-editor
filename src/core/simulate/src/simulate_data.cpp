#include "simulate_data.hpp"

namespace sme::simulate {

void SimulationData::clear() {
  timePoints.clear();
  concentration.clear();
  avgMinMax.clear();
  concentrationMax.clear();
  concPadding.clear();
  xmlModel.clear();
}

void SimulationData::pop_back(){
  timePoints.pop_back();
  concentration.pop_back();
  avgMinMax.pop_back();
  concentrationMax.pop_back();
  concPadding.pop_back();
}

} // namespace sme::simulate
