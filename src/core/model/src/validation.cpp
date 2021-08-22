#include "validation.hpp"
#include "logger.hpp"
#include <QString>
#include <sbml/SBMLTypes.h>
#include <sbml/extension/SBMLDocumentPlugin.h>
#include <sbml/packages/spatial/common/SpatialExtensionTypes.h>
#include <sbml/packages/spatial/extension/SpatialExtension.h>

namespace sme::model {

void printSBMLDocWarnings(const libsbml::SBMLDocument *doc) {
  auto severity = libsbml::LIBSBML_SEV_WARNING;
  unsigned n = doc->getNumErrors(severity);
  for (unsigned i = 0; i < n; ++i) {
    const auto *err = doc->getErrorWithSeverity(i, severity);
    SPDLOG_WARN("[{}] line {}:{} {}", err->getCategoryAsString(),
                err->getLine(), err->getColumn(), err->getMessage());
  }
}

std::string countAndPrintSBMLDocErrors(const libsbml::SBMLDocument *doc) {
  std::string errors{};
  auto severity{libsbml::LIBSBML_SEV_ERROR};
  const unsigned n{doc->getNumErrors(severity)};
  for (unsigned i = 0; i < n; ++i) {
    const auto *err{doc->getErrorWithSeverity(i, severity)};
    if (err == nullptr) {
      SPDLOG_WARN("{} (libsbml getErrorWithSeverity returned a nullptr)",
                  errors);
      errors = "Failed to read file: invalid or corrupted";
      return errors;
    }
    if (err->getErrorId() == 1221608) {
      // ignore this error for now:
      // https://github.com/spatial-model-editor/spatial-model-editor/issues/465
      SPDLOG_WARN("Ignoring this libSBML error:\n[{}] [{}] line {}:{} {}",
                  err->getErrorId(), err->getCategoryAsString(), err->getLine(),
                  err->getColumn(), err->getMessage());
    } else {
      auto s{fmt::format("[{}] [{}] line {}:{} {}", err->getErrorId(),
                         err->getCategoryAsString(), err->getLine(),
                         err->getColumn(), err->getMessage())};
      SPDLOG_ERROR("{}", s);
      errors.append(s);
      errors.append("\n");
    }
  }
  return errors;
}

std::string validateAndUpgradeSBMLDoc(libsbml::SBMLDocument *doc) {
  auto errors{countAndPrintSBMLDocErrors(doc)};
  if (!errors.empty()) {
    SPDLOG_ERROR("Errors while reading SBML file, aborting.");
    return errors;
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
      countAndPrintSBMLDocErrors(doc);
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
  printSBMLDocWarnings(doc);
  return {};
}

} // namespace sme::model
