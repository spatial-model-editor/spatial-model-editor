#include "validation.hpp"
#include "logger.hpp"
#include <QString>
#include <sbml/SBMLTypes.h>
#include <sbml/extension/SBMLDocumentPlugin.h>
#include <sbml/packages/spatial/common/SpatialExtensionTypes.h>
#include <sbml/packages/spatial/extension/SpatialExtension.h>

namespace sme {

namespace model {

void printSBMLDocWarnings(const libsbml::SBMLDocument *doc) {
  auto severity = libsbml::LIBSBML_SEV_WARNING;
  unsigned n = doc->getNumErrors(severity);
  for (unsigned i = 0; i < n; ++i) {
    const auto *err = doc->getErrorWithSeverity(i, severity);
    SPDLOG_WARN("[{}] line {}:{} {}", err->getCategoryAsString(),
                err->getLine(), err->getColumn(), err->getMessage());
  }
}

void printSBMLDocErrors(const libsbml::SBMLDocument *doc) {
  auto severity = libsbml::LIBSBML_SEV_ERROR;
  unsigned n = doc->getNumErrors(severity);
  for (unsigned i = 0; i < n; ++i) {
    const auto *err = doc->getErrorWithSeverity(i, severity);
    SPDLOG_ERROR("[{}] line {}:{} {}", err->getCategoryAsString(),
                 err->getLine(), err->getColumn(), err->getMessage());
  }
}

bool validateAndUpgradeSBMLDoc(libsbml::SBMLDocument *doc) {
  if (doc->getErrorLog()->getNumFailsWithSeverity(libsbml::LIBSBML_SEV_ERROR) >
      0) {
    SPDLOG_ERROR("Errors while reading SBML file");
    printSBMLDocErrors(doc);
    return false;
  }
  SPDLOG_INFO("Successfully imported SBML Level {}, Version {} model",
              doc->getLevel(), doc->getVersion());
  // upgrade SBML document to latest version
  auto lvl = libsbml::SBMLDocument::getDefaultLevel();
  auto ver = libsbml::SBMLDocument::getDefaultVersion();
  if (!(doc->getLevel() == lvl && doc->getVersion() == ver)) {
    if (doc->setLevelAndVersion(lvl, ver)) {
      SPDLOG_INFO("Successfully upgraded SBML model to Level {}, Version {}",
                  doc->getLevel(), doc->getVersion());
    } else {
      SPDLOG_ERROR(
          "Error - failed to upgrade SBML file (continuing anyway...)");
      printSBMLDocErrors(doc);
    }
  }
  // enable spatial extension
  if (!doc->isPackageEnabled("spatial")) {
    doc->enablePackage(libsbml::SpatialExtension::getXmlnsL3V1V1(), "spatial",
                       true);
    doc->setPackageRequired("spatial", true);
    SPDLOG_INFO("Enabling spatial extension");
  }
  doc->checkConsistency();
  printSBMLDocErrors(doc);
  printSBMLDocWarnings(doc);
  return true;
}

} // namespace model

} // namespace sme
