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
 * @brief Returns whether compartment has any non-constant species.
 */
bool compartmentContainsNonConstantSpecies(const model::Model &model,
                                           const QString &compId);

/**
 * @brief Non-constant species ids in compartment.
 */
std::vector<std::string> getNonConstantSpecies(const model::Model &model,
                                               const QString &compId);

} // namespace sme::simulate
