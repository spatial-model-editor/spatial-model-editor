// DUNE-copasi ini file generation implementation

#pragma once

#include "simulate_options.hpp"
#include "model.hpp"
#include <QString>
#include <vector>
#include <string>

namespace simulate {

std::vector<std::string>
makeValidDuneSpeciesNames(const std::vector<std::string> &names);

bool compartmentContainsNonConstantSpecies(const model::Model &model,
                                                  const QString &compId);

std::vector<std::string> getNonConstantSpecies(const model::Model &model,
                                                      const QString &compId);

bool modelHasIndependentCompartments(const model::Model &model);

}
