// SimulatorData

#pragma once

#include "simulate_options.hpp"
#include <string>
#include <vector>
#include <cereal/cereal.hpp>
#include <cereal/types/string.hpp>

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
  [[nodiscard]] std::size_t size() const;
  void reserve(std::size_t n);
  void pop_back();

  template <class Archive>
  void serialize(Archive &ar, std::uint32_t const version) {
    if (version == 0) {
      ar(timePoints, concentration, avgMinMax,
         concentrationMax, concPadding, xmlModel);
    }
  }
};

} // namespace sme::simulate

CEREAL_CLASS_VERSION(sme::simulate::SimulationData, 0);
