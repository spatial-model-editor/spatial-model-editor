// DUNE-copasi ini file generation implementation

#pragma once

#include "model.hpp"
#include "simulate_options.hpp"
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

bool modelHasIndependentCompartments(const model::Model &model);

} // namespace sme::simulate
