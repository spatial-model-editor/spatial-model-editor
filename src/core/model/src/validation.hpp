// SBML sId and Name utility functions

#pragma once

#include <string>

namespace libsbml {
class SBMLDocument;
}

namespace sme::model {

struct ValidateAndUpgradeResult {
  std::string errors{};
  bool spatial{true};
};

void printSBMLDocWarnings(const libsbml::SBMLDocument *doc);
std::string countAndPrintSBMLDocErrors(const libsbml::SBMLDocument *doc);
ValidateAndUpgradeResult validateAndUpgradeSBMLDoc(libsbml::SBMLDocument *doc);

} // namespace sme::model
