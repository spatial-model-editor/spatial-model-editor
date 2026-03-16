// DUNE-copasi ini file generation implementation

#pragma once

#include "sme/model.hpp"
#include "sme/simulate_options.hpp"
#include <QString>
#include <string>
#include <vector>

namespace sme::simulate {

/**
 * @brief Convert species names into DUNE-compatible identifiers.
 */
std::vector<std::string>
makeValidDuneSpeciesNames(const std::vector<std::string> &names);

/**
 * @brief Returns whether compartment has any simulated species.
 */
bool compartmentContainsSimulatedSpecies(const model::Model &model,
                                         const QString &compId);

/**
 * @brief Simulated species ids in compartment.
 */
std::vector<std::string> getSimulatedSpecies(const model::Model &model,
                                             const QString &compId);

} // namespace sme::simulate
