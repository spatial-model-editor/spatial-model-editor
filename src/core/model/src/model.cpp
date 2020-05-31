#include "model.hpp"

#include <sbml/SBMLTypes.h>
#include <sbml/extension/SBMLDocumentPlugin.h>
#include <sbml/packages/spatial/common/SpatialExtensionTypes.h>
#include <sbml/packages/spatial/extension/SpatialExtension.h>

#include <algorithm>
#include <stdexcept>
#include <utility>

#include "id.hpp"
#include "logger.hpp"
#include "math.hpp"
#include "mesh.hpp"
#include "utils.hpp"
#include "validation.hpp"
#include "xml_annotation.hpp"

namespace model {

Model::Model() = default;

Model::~Model() = default;

void Model::createSBMLFile(const std::string &name) {
  clear();
  SPDLOG_INFO("Creating new SBML model '{}'...", name);
  doc = std::make_unique<libsbml::SBMLDocument>(libsbml::SBMLDocument());
  doc->createModel(name);
  currentFilename = name.c_str();
  if (currentFilename.right(4) != ".xml") {
    currentFilename.append(".xml");
  }
  initModelData();
}

void Model::importSBMLFile(const std::string &filename) {
  clear();
  currentFilename = filename.c_str();
  SPDLOG_INFO("Loading SBML file {}...", filename);
  doc.reset(libsbml::readSBMLFromFile(filename.c_str()));
  initModelData();
}

void Model::importSBMLString(const std::string &xml) {
  clear();
  SPDLOG_INFO("Importing SBML from string...");
  doc.reset(libsbml::readSBMLFromString(xml.c_str()));
  initModelData();
}

void Model::initModelData() {
  isValid = validateAndUpgradeSBMLDoc(doc.get());
  if (!isValid) {
    return;
  }
  auto *model = doc->getModel();
  modelParameters = ModelParameters(model);
  modelFunctions = ModelFunctions(model);
  modelMembranes.clear();
  // todo: reduce these cyclic dependencies: currently order of initialization
  // matters, should be possible to reduce coupling here
  modelCompartments =
      ModelCompartments(model, &modelGeometry, &modelMembranes, &modelSpecies);
  modelGeometry = ModelGeometry(model, &modelCompartments, &modelMembranes);
  modelGeometry.importSampledFieldGeometry(model);
  modelGeometry.importParametricGeometry(model);
  modelSpecies = ModelSpecies(model, &modelCompartments, &modelGeometry,
                              &modelParameters, &modelReactions);
  modelReactions = ModelReactions(model, modelMembranes.getMembranes());
  modelUnits = ModelUnits(model);
}

bool Model::getIsValid() const { return isValid; }

const QString &Model::getCurrentFilename() const { return currentFilename; }

void Model::exportSBMLFile(const std::string &filename) {
  if (!isValid) {
    return;
  }
  modelGeometry.writeGeometryToSBML();
  modelMembranes.exportToSBML(doc->getModel());
  SPDLOG_INFO("Exporting SBML model to {}", filename);
  currentFilename = filename.c_str();
  if (!libsbml::SBMLWriter().writeSBML(doc.get(), filename)) {
    SPDLOG_ERROR("Failed to write to {}", filename);
  }
}

QString Model::getXml() {
  QString xml;
  if (!isValid) {
    return {};
  }
  modelGeometry.writeGeometryToSBML();
  modelMembranes.exportToSBML(doc->getModel());
  printSBMLDocErrors(doc.get());
  std::unique_ptr<char, decltype(&std::free)> xmlChar(
      libsbml::writeSBMLToString(doc.get()), &std::free);
  xml = QString(xmlChar.get());
  return xml;
}

void Model::setName(const QString &name) {
  doc->getModel()->setName(name.toStdString());
}

QString Model::getName() const { return doc->getModel()->getName().c_str(); }

ModelCompartments &Model::getCompartments() { return modelCompartments; }

const ModelCompartments &Model::getCompartments() const {
  return modelCompartments;
}

ModelGeometry &Model::getGeometry() { return modelGeometry; }

const ModelGeometry &Model::getGeometry() const { return modelGeometry; }

ModelMembranes &Model::getMembranes() { return modelMembranes; }

const ModelMembranes &Model::getMembranes() const { return modelMembranes; }

ModelSpecies &Model::getSpecies() { return modelSpecies; }

const ModelSpecies &Model::getSpecies() const { return modelSpecies; }

ModelReactions &Model::getReactions() { return modelReactions; }

const ModelReactions &Model::getReactions() const { return modelReactions; }

ModelFunctions &Model::getFunctions() { return modelFunctions; }

const ModelFunctions &Model::getFunctions() const { return modelFunctions; }

ModelParameters &Model::getParameters() { return modelParameters; }

const ModelParameters &Model::getParameters() const { return modelParameters; }

ModelUnits &Model::getUnits() { return modelUnits; }

const ModelUnits &Model::getUnits() const { return modelUnits; }

void Model::clear() {
  doc.reset();
  isValid = false;
  currentFilename.clear();
  modelCompartments = ModelCompartments{};
  modelGeometry = ModelGeometry{};
  modelMembranes.clear();
  modelSpecies = ModelSpecies{};
  modelReactions = ModelReactions{};
  modelFunctions = ModelFunctions{};
  modelParameters = ModelParameters{};
  modelUnits = ModelUnits{};
}

SpeciesGeometry Model::getSpeciesGeometry(const QString &speciesID) const {
  return {modelGeometry.getImage().size(),
          modelSpecies.getField(speciesID)->getCompartment()->getPixels(),
          modelGeometry.getPhysicalOrigin(), modelGeometry.getPixelWidth(),
          getUnits()};
}

std::string Model::inlineExpr(const std::string &mathExpression) const {
  std::string inlined;
  // inline any Function calls in expr
  inlined = inlineFunctions(mathExpression, doc->getModel());
  // inline any Assignment Rules in expr
  inlined = inlineAssignments(inlined, doc->getModel());
  return inlined;
}

std::string Model::getRateRule(const std::string &speciesID) const {
  const auto *rule = doc->getModel()->getRateRule(speciesID);
  if (rule != nullptr) {
    return inlineExpr(rule->getFormula());
  }
  return {};
}

} // namespace model
