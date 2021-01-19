// SBML sId and Name utility functions

#pragma once

namespace libsbml {
class SBMLDocument;
}

namespace sme {

namespace model {

void printSBMLDocWarnings(const libsbml::SBMLDocument *doc);
void printSBMLDocErrors(const libsbml::SBMLDocument *doc);
bool validateAndUpgradeSBMLDoc(libsbml::SBMLDocument *doc);

} // namespace model

} // namespace sme
