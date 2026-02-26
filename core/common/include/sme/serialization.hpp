// Load/save functionality using cereal

#pragma once
#include "sme/model_settings.hpp"
#include "sme/simulate_data.hpp"
#include "sme/simulate_options.hpp"

namespace sme::common {

/**
 * @brief Contents of an ``.sme`` project file.
 */
struct SmeFileContents {
  /**
   * @brief SBML model XML.
   */
  std::string xmlModel{};
  /**
   * @brief Optional cached simulation results.
   */
  std::unique_ptr<simulate::SimulationData> simulationData{};
};

/**
 * @brief Read an ``.sme`` file from disk.
 *
 * @returns Parsed contents, or ``nullptr`` if loading fails.
 */
std::unique_ptr<SmeFileContents> importSmeFile(const std::string &filename);
/**
 * @brief Write an ``.sme`` file to disk.
 *
 * @returns ``true`` on success.
 */
bool exportSmeFile(const std::string &filename,
                   const SmeFileContents &contents);

/**
 * @brief Serialize model settings to XML.
 */
std::string toXml(const model::Settings &sbmlAnnotation);
/**
 * @brief Parse model settings from XML.
 */
model::Settings fromXml(const std::string &xml);

} // namespace sme::common
