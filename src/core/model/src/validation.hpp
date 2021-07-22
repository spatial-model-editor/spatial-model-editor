// SBML sId and Name utility functions

#pragma once

#include <string>

namespace libsbml {
class SBMLDocument;
}

namespace sme::model {

void printSBMLDocWarnings(const libsbml::SBMLDocument *doc);
std::string countAndPrintSBMLDocErrors(const libsbml::SBMLDocument *doc);
std::string validateAndUpgradeSBMLDoc(libsbml::SBMLDocument *doc);

} // namespace sme::model
