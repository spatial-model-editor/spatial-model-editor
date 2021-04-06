// SimulatorData

#pragma once

#include "simulate_options.hpp"
#include <string>
#include <vector>

namespace sme::simulate {

class SimulationData {
public:
  std::vector<double> timePoints;
  // time->compartment->(ix->species)
  std::vector<std::vector<std::vector<double>>> concentration;
  // time->compartment->species
  std::vector<std::vector<std::vector<AvgMinMax>>> avgMinMax;
  // time->compartment->species
  std::vector<std::vector<std::vector<double>>> concentrationMax;
  // time->concPadding
  std::vector<std::size_t> concPadding;
  std::string xmlModel;
  void clear();
  void pop_back();
};

} // namespace sme::simulate
