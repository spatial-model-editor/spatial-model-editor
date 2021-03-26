// SBML sId and Name utility functions

#pragma once

namespace libsbml {
class SBMLDocument;
}

namespace sme::model {

void printSBMLDocWarnings(const libsbml::SBMLDocument *doc);
int countAndPrintSBMLDocErrors(const libsbml::SBMLDocument *doc);
bool validateAndUpgradeSBMLDoc(libsbml::SBMLDocument *doc);

} // namespace sme::model
