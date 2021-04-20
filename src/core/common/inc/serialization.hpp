// Load/save functionality using cereal

#pragma once

#include "simulate_data.hpp"
#include "simulate_options.hpp"

namespace sme::utils {

struct SmeFileContents {
  std::string xmlModel;
  simulate::SimulationData simulationData;
  simulate::SimulationSettings simulationSettings;
};

SmeFileContents importSmeFile(const std::string &filename);
bool exportSmeFile(const std::string &filename, const SmeFileContents& contents);

} // namespace sme::utils
