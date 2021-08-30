// Load/save functionality using cereal

#pragma once
#include "model_settings.hpp"
#include "simulate_data.hpp"
#include "simulate_options.hpp"

namespace sme::common {

struct SmeFileContents {
  std::string xmlModel{};
  std::unique_ptr<simulate::SimulationData> simulationData{};
};

std::unique_ptr<SmeFileContents> importSmeFile(const std::string &filename);
bool exportSmeFile(const std::string &filename,
                   const SmeFileContents &contents);

std::string toXml(const model::Settings &sbmlAnnotation);
model::Settings fromXml(const std::string &xml);

} // namespace sme::common
