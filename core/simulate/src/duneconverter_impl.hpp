// DUNE-copasi ini file generation implementation

#pragma once

#include "sme/model.hpp"
#include "sme/simulate_options.hpp"
#include <QString>
#include <string>
#include <vector>

namespace sme::simulate {

std::vector<std::string>
makeValidDuneSpeciesNames(const std::vector<std::string> &names);

bool compartmentContainsNonConstantSpecies(const model::Model &model,
                                           const QString &compId);

std::vector<std::string> getNonConstantSpecies(const model::Model &model,
                                               const QString &compId);

} // namespace sme::simulate
