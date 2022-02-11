#include "sme/model.hpp"
#include "id.hpp"
#include "sbml_math.hpp"
#include "sme/logger.hpp"
#include "sme/mesh.hpp"
#include "sme/utils.hpp"
#include "sme/xml_annotation.hpp"
#include "validation.hpp"
#include <QFileInfo>
#include <algorithm>
#include <sbml/SBMLTransforms.h>
#include <sbml/SBMLTypes.h>
#include <sbml/extension/SBMLDocumentPlugin.h>
#include <sbml/packages/spatial/common/SpatialExtensionTypes.h>
#include <sbml/packages/spatial/extension/SpatialExtension.h>
#include <stdexcept>
#include <utility>

namespace sme::model {

Model::Model() { clear(); }

Model::~Model() = default;

void Model::createSBMLFile(const std::string &name) {
  clear();
  SPDLOG_INFO("Creating new SBML model '{}'...", name);
  doc = std::make_unique<libsbml::SBMLDocument>(libsbml::SBMLDocument());
  doc->createModel(name);
  currentFilename = name.c_str();
  initModelData(true);
}

void Model::importSBMLFile(const std::string &filename) {
  clear();
  currentFilename = QFileInfo(filename.c_str()).baseName();
  SPDLOG_INFO("Loading SBML file {}...", filename);
  doc.reset(libsbml::readSBMLFromFile(filename.c_str()));
  initModelData();
  setHasUnsavedChanges(false);
}

void Model::importSBMLString(const std::string &xml,
                             const std::string &filename) {
  clear();
  currentFilename = QFileInfo(filename.c_str()).baseName();
  SPDLOG_INFO("Importing SBML from string...");
  doc.reset(libsbml::readSBMLFromString(xml.c_str()));
  initModelData();
  setHasUnsavedChanges(false);
}

void Model::initModelData(bool emptySpatialModel) {
  auto validateAndUpgradeResult{validateAndUpgradeSBMLDoc(doc.get())};
  if (emptySpatialModel) {
    validateAndUpgradeResult.spatial = true;
  }
  errorMessage = validateAndUpgradeResult.errors.c_str();
  isValid = errorMessage.isEmpty();
  const auto isNonSpatialModel{!validateAndUpgradeResult.spatial};
  if (!isValid) {
    return;
  }
  auto *model{doc->getModel()};
  settings = std::make_unique<Settings>(getSbmlAnnotation(model));
  modelUnits = std::make_unique<ModelUnits>(model);
  modelMath = std::make_unique<ModelMath>(model);
  modelFunctions = std::make_unique<ModelFunctions>(model);
  modelMembranes = std::make_unique<ModelMembranes>(model);
  // todo: reduce these cyclic dependencies: currently order of initialization
  // matters, should be possible to reduce coupling here
  modelCompartments = std::make_unique<ModelCompartments>(
      model, modelMembranes.get(), modelUnits.get(),
      smeFileContents->simulationData.get());
  modelGeometry = std::make_unique<ModelGeometry>(
      model, modelCompartments.get(), modelMembranes.get(), modelUnits.get(),
      settings.get());
  modelCompartments->setGeometryPtr(modelGeometry.get());
  modelGeometry->importSampledFieldGeometry(model);
  modelParameters = std::make_unique<ModelParameters>(model);
  modelSpecies = std::make_unique<ModelSpecies>(
      model, modelCompartments.get(), modelGeometry.get(),
      modelParameters.get(), smeFileContents->simulationData.get(),
      settings.get());
  modelCompartments->setSpeciesPtr(modelSpecies.get());
  modelEvents = std::make_unique<ModelEvents>(model, modelParameters.get(),
                                              modelSpecies.get());
  modelParameters->setEventsPtr(modelEvents.get());
  modelReactions = std::make_unique<ModelReactions>(
      model, modelCompartments.get(), modelMembranes.get(), isNonSpatialModel);
  modelCompartments->setReactionsPtr(modelReactions.get());
  modelSpecies->setReactionsPtr(modelReactions.get());
}

void Model::setHasUnsavedChanges(bool unsavedChanges) {
  modelUnits->setHasUnsavedChanges(unsavedChanges);
  modelFunctions->setHasUnsavedChanges(unsavedChanges);
  modelMembranes->setHasUnsavedChanges(unsavedChanges);
  modelCompartments->setHasUnsavedChanges(unsavedChanges);
  modelGeometry->setHasUnsavedChanges(unsavedChanges);
  modelParameters->setHasUnsavedChanges(unsavedChanges);
  modelSpecies->setHasUnsavedChanges(unsavedChanges);
  modelReactions->setHasUnsavedChanges(unsavedChanges);
  modelEvents->setHasUnsavedChanges(unsavedChanges);
}

bool Model::getIsValid() const { return isValid; }

const QString &Model::getErrorMessage() const { return errorMessage; }

bool Model::getHasUnsavedChanges() const {
  return modelUnits->getHasUnsavedChanges() ||
         modelFunctions->getHasUnsavedChanges() ||
         modelMembranes->getHasUnsavedChanges() ||
         modelCompartments->getHasUnsavedChanges() ||
         modelGeometry->getHasUnsavedChanges() ||
         modelParameters->getHasUnsavedChanges() ||
         modelSpecies->getHasUnsavedChanges() ||
         modelReactions->getHasUnsavedChanges() ||
         modelEvents->getHasUnsavedChanges();
}

const QString &Model::getCurrentFilename() const { return currentFilename; }

void Model::exportSBMLFile(const std::string &filename) {
  if (!isValid) {
    return;
  }
  updateSBMLDoc();
  SPDLOG_INFO("Exporting SBML model to {}", filename);
  if (!libsbml::SBMLWriter().writeSBML(doc.get(), filename)) {
    SPDLOG_ERROR("Failed to write to {}", filename);
    return;
  }
  setHasUnsavedChanges(false);
}

void Model::importFile(const std::string &filename) {
  clear();
  currentFilename = QFileInfo(filename.c_str()).baseName();
  SPDLOG_INFO("Importing file {} ...", filename);
  auto importedSmeFileContents{common::importSmeFile(filename)};
  if (importedSmeFileContents != nullptr) {
    SPDLOG_INFO("  -> SME file", filename);
    doc.reset(
        libsbml::readSBMLFromString(importedSmeFileContents->xmlModel.c_str()));
  } else {
    SPDLOG_INFO("  -> SBML file", filename);
    doc.reset(libsbml::readSBMLFromFile(filename.c_str()));
  }
  initModelData();
  if (importedSmeFileContents != nullptr) {
    smeFileContents->simulationData =
        std::move(importedSmeFileContents->simulationData);
    modelCompartments->setSimulationDataPtr(
        smeFileContents->simulationData.get());
    modelSpecies->setSimulationDataPtr(smeFileContents->simulationData.get());
  }
  setHasUnsavedChanges(false);
}

void Model::exportSMEFile(const std::string &filename) {
  currentFilename = filename.c_str();
  if (auto len{currentFilename.lastIndexOf(".")}; len > 0) {
    currentFilename.truncate(len);
  }
  updateSBMLDoc();
  smeFileContents->xmlModel = getXml().toStdString();
  if (!common::exportSmeFile(filename, *smeFileContents)) {
    SPDLOG_WARN("Failed to save file '{}'", filename);
  }
  setHasUnsavedChanges(false);
}

void Model::updateSBMLDoc() {
  modelGeometry->writeGeometryToSBML();
  setSbmlAnnotation(doc->getModel(), *settings);
  modelMembranes->exportToSBML(modelGeometry->getPixelWidth() *
                               modelGeometry->getPixelDepth());
}

QString Model::getXml() {
  QString xml;
  if (!isValid) {
    return {};
  }
  updateSBMLDoc();
  countAndPrintSBMLDocErrors(doc.get());
  std::unique_ptr<char, decltype(&std::free)> xmlChar(
      libsbml::writeSBMLToString(doc.get()), &std::free);
  xml = QString(xmlChar.get());
  return xml;
}

void Model::setName(const QString &name) {
  doc->getModel()->setName(name.toStdString());
}

QString Model::getName() const { return doc->getModel()->getName().c_str(); }

ModelCompartments &Model::getCompartments() { return *modelCompartments; }

const ModelCompartments &Model::getCompartments() const {
  return *modelCompartments;
}

ModelGeometry &Model::getGeometry() { return *modelGeometry; }

const ModelGeometry &Model::getGeometry() const { return *modelGeometry; }

ModelMembranes &Model::getMembranes() { return *modelMembranes; }

const ModelMembranes &Model::getMembranes() const { return *modelMembranes; }

ModelSpecies &Model::getSpecies() { return *modelSpecies; }

const ModelSpecies &Model::getSpecies() const { return *modelSpecies; }

ModelReactions &Model::getReactions() { return *modelReactions; }

const ModelReactions &Model::getReactions() const { return *modelReactions; }

ModelFunctions &Model::getFunctions() { return *modelFunctions; }

const ModelFunctions &Model::getFunctions() const { return *modelFunctions; }

ModelParameters &Model::getParameters() { return *modelParameters; }

const ModelParameters &Model::getParameters() const { return *modelParameters; }

ModelEvents &Model::getEvents() { return *modelEvents; }

const ModelEvents &Model::getEvents() const { return *modelEvents; }

ModelUnits &Model::getUnits() { return *modelUnits; }

const ModelUnits &Model::getUnits() const { return *modelUnits; }

ModelMath &Model::getMath() { return *modelMath; }

const ModelMath &Model::getMath() const { return *modelMath; }

simulate::SimulationData &Model::getSimulationData() {
  return *smeFileContents->simulationData;
}

const simulate::SimulationData &Model::getSimulationData() const {
  return *smeFileContents->simulationData;
}

SimulationSettings &Model::getSimulationSettings() {
  return settings->simulationSettings;
}

const SimulationSettings &Model::getSimulationSettings() const {
  return settings->simulationSettings;
}

MeshParameters &Model::getMeshParameters() { return settings->meshParameters; }

const MeshParameters &Model::getMeshParameters() const {
  return settings->meshParameters;
}

void Model::clear() {
  doc.reset();
  isValid = false;
  errorMessage.clear();
  currentFilename.clear();
  modelCompartments = std::make_unique<ModelCompartments>();
  modelGeometry = std::make_unique<ModelGeometry>();
  modelMembranes = std::make_unique<ModelMembranes>();
  modelSpecies = std::make_unique<ModelSpecies>();
  modelReactions = std::make_unique<ModelReactions>();
  modelFunctions = std::make_unique<ModelFunctions>();
  modelParameters = std::make_unique<ModelParameters>();
  modelEvents = std::make_unique<ModelEvents>();
  modelUnits = std::make_unique<ModelUnits>();
  modelMath = std::make_unique<ModelMath>();
  smeFileContents = std::make_unique<common::SmeFileContents>();
  smeFileContents->simulationData =
      std::make_unique<simulate::SimulationData>();
  settings = std::make_unique<Settings>();
}

SpeciesGeometry Model::getSpeciesGeometry(const QString &speciesID) const {
  return {modelGeometry->getImage().size(),
          modelSpecies->getField(speciesID)->getCompartment()->getPixels(),
          modelGeometry->getPhysicalOrigin(), modelGeometry->getPixelWidth(),
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

DisplayOptions Model::getDisplayOptions() const {
  return settings->displayOptions;
}

void Model::setDisplayOptions(const DisplayOptions &displayOptions) {
  settings->displayOptions = displayOptions;
}

} // namespace sme::model
