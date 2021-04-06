// Load/save functionality using cereal

#pragma once

#include "simulate_data.hpp"
#include "simulate_options.hpp"

namespace sme::utils {

struct SmeFileContents {
  std::string xmlModel;
  simulate::SimulationData simulationData;
};

class SmeFile {
private:
  SmeFileContents contents;

public:
  SmeFile();
  SmeFile(const std::string &model, const sme::simulate::SimulationData &data);
  [[nodiscard]] const std::string &xmlModel() const;
  void setXmlModel(const std::string& xmlModel);
  simulate::SimulationData &simulationData();
  bool importFile(const std::string &filename);
  bool exportFile(const std::string &filename) const;
};

} // namespace sme::utils
