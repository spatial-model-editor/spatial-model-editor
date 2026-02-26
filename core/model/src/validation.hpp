// SBML sId and Name utility functions

#pragma once

#include <string>

namespace libsbml {
class SBMLDocument;
}

namespace sme::model {

/**
 * @brief Result of SBML validation and auto-upgrade pass.
 */
struct ValidateAndUpgradeResult {
  /**
   * @brief Validation errors as human-readable text.
   */
  std::string errors{};
  /**
   * @brief Whether model remains spatial after upgrade.
   */
  bool spatial{true};
};

/**
 * @brief Print libSBML warnings for a document.
 */
void printSBMLDocWarnings(const libsbml::SBMLDocument *doc);
/**
 * @brief Count and print libSBML errors.
 */
std::string countAndPrintSBMLDocErrors(const libsbml::SBMLDocument *doc);
/**
 * @brief Validate and upgrade SBML document to supported representation.
 */
ValidateAndUpgradeResult validateAndUpgradeSBMLDoc(libsbml::SBMLDocument *doc);

} // namespace sme::model
