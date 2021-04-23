#include "model_species.hpp"
#include "id.hpp"
#include "logger.hpp"
#include "model_compartments.hpp"
#include "model_geometry.hpp"
#include "model_parameters.hpp"
#include "model_reactions.hpp"
#include "sbml_math.hpp"
#include "sbml_utils.hpp"
#include "simulate_data.hpp"
#include "utils.hpp"
#include "xml_annotation.hpp"
#include <QString>
#include <memory>
#include <sbml/SBMLTypes.h>
#include <sbml/extension/SBMLDocumentPlugin.h>
#include <sbml/packages/spatial/common/SpatialExtensionTypes.h>
#include <sbml/packages/spatial/extension/SpatialExtension.h>

namespace sme {

namespace model {

static QStringList importIds(const libsbml::Model *model) {
  QStringList ids;
  for (unsigned int i = 0; i < model->getNumSpecies(); ++i) {
    const auto *spec = model->getSpecies(i);
    ids.push_back(spec->getId().c_str());
    if (spec->isSetHasOnlySubstanceUnits() &&
        spec->getHasOnlySubstanceUnits()) {
      SPDLOG_WARN("Species {} hasOnlySubstanceUnits=true : meaning unclear in "
                  "spatial PDE context, for now we just ignore this",
                  spec->getId());
    }
  }
  return ids;
}

static QStringList importNamesAndMakeUnique(libsbml::Model *model) {
  QStringList names;
  // get all species, make a list for each compartment
  for (unsigned int i = 0; i < model->getNumSpecies(); ++i) {
    auto *spec = model->getSpecies(i);
    const auto &sId = spec->getId();
    if (spec->getName().empty()) {
      SPDLOG_INFO("Species '{}' has no Name, using sId", sId);
      spec->setName(sId);
    }
    std::string name = spec->getName();
    while (names.contains(name.c_str())) {
      const auto &compartmentId = spec->getCompartment();
      const auto &compartmentName =
          model->getCompartment(compartmentId)->getName();
      name.append("_").append(compartmentName);
      SPDLOG_INFO("Changing Species '{}' name to '{}' to make it unique", sId,
                  name);
    }
    spec->setName(name);
    names.push_back(QString::fromStdString(name));
  }
  return names;
}

static QStringList importCompartmentIds(const libsbml::Model *model) {
  QStringList ids;
  for (unsigned int i = 0; i < model->getNumSpecies(); ++i) {
    const auto *spec = model->getSpecies(i);
    ids.push_back(spec->getCompartment().c_str());
  }
  return ids;
}

static void makeInitialConcentrationsValid(libsbml::Model *model) {
  for (unsigned int i = 0; i < model->getNumSpecies(); ++i) {
    auto *spec = model->getSpecies(i);
    if (!spec->isSetInitialConcentration()) {
      SPDLOG_WARN("Species '{}' initialConcentration is not set:",
                  spec->getId());
      double newInitialConcentration = 0;
      if (spec->isSetInitialAmount()) {
        // convert initial amount to initial concentration
        double initialAmount = spec->getInitialAmount();
        const auto *comp = model->getCompartment(spec->getCompartment());
        double compartmentSize = comp->getSize();
        newInitialConcentration = initialAmount / compartmentSize;
        SPDLOG_WARN("  - using initialAmount / compartmentSize = {} / {} = {}",
                    initialAmount, compartmentSize, newInitialConcentration);
        spec->setInitialConcentration(newInitialConcentration);
      } else {
        // if no initial condition is set, just use zero as default
        SPDLOG_WARN("  - initialAmount also not set, using {}",
                    newInitialConcentration);
        spec->setInitialConcentration(newInitialConcentration);
      }
    }
  }
}

void ModelSpecies::removeInitialAssignment(const QString &id) {
  hasUnsavedChanges = true;
  if (auto sampledFieldID = getSampledFieldInitialAssignment(id);
      !sampledFieldID.isEmpty()) {
    auto *geom = getOrCreateGeometry(sbmlModel);
    if (std::unique_ptr<libsbml::SampledField> sf(
            geom->removeSampledField(sampledFieldID.toStdString()));
        sf != nullptr) {
      SPDLOG_INFO("removed SampledField {}", sf->getId());
    }
    // remove parameter with spatialref to sampled field
    std::string paramID =
        sbmlModel->getInitialAssignmentBySymbol(id.toStdString())
            ->getMath()
            ->getName();
    std::unique_ptr<libsbml::Parameter> p(sbmlModel->removeParameter(paramID));
    SPDLOG_INFO("removed Parameter {}", p->getId());
  }
  if (std::unique_ptr<libsbml::InitialAssignment> ia(
          sbmlModel->removeInitialAssignment(id.toStdString()));
      ia != nullptr) {
    SPDLOG_INFO("removed InitialAssignment {}", ia->getId());
  }
}

void ModelSpecies::setFieldConcAnalytic(geometry::Field &field,
                                        const std::string &expr) {
  SPDLOG_INFO("expr: {}", expr);
  auto inlinedExpr = inlineFunctions(expr, sbmlModel);
  inlinedExpr = inlineAssignments(inlinedExpr, sbmlModel);
  SPDLOG_INFO("  - inlined expr: {}", inlinedExpr);
  std::string xId{modelParameters->getSpatialCoordinates().x.id};
  std::string yId{modelParameters->getSpatialCoordinates().y.id};
  std::map<const std::string, std::pair<double, bool>> sbmlVars;
  auto &xCoordPair = sbmlVars[xId];
  xCoordPair = {0, false};
  double &xCoord = xCoordPair.first;
  auto &yCoordPair = sbmlVars[yId];
  yCoordPair = {0, false};
  double &yCoord = yCoordPair.first;
  auto astExpr = mathStringToAST(inlinedExpr);
  SPDLOG_TRACE("  - parsed expr: {}", mathASTtoString(astExpr.get()));
  if (astExpr == nullptr) {
    SPDLOG_ERROR("Failed to parse expression '{}'", inlinedExpr);
    return;
  }
  hasUnsavedChanges = true;
  const auto &origin = modelGeometry->getPhysicalOrigin();
  double pixelWidth = modelGeometry->getPixelWidth();
  int imgHeight = field.getCompartment()->getCompartmentImage().height();
  for (std::size_t i = 0; i < field.getCompartment()->nPixels(); ++i) {
    // position in pixels (with (0,0) in top-left of image):
    const auto &point = field.getCompartment()->getPixel(i);
    // rescale to physical x,y point (with (0,0) in bottom-left):
    xCoord = origin.x() + pixelWidth * (static_cast<double>(point.x()) + 0.5);
    int y = imgHeight - 1 - point.y();
    yCoord = origin.y() + pixelWidth * (static_cast<double>(y) + 0.5);
    double conc = evaluateMathAST(astExpr.get(), sbmlVars, sbmlModel);
    field.setConcentration(i, conc);
  }
  field.setIsUniformConcentration(false);
}

std::vector<double>
ModelSpecies::getSampledFieldConcentrationFromSBML(const QString &id) const {
  std::vector<double> array;
  std::string sampledFieldID =
      getSampledFieldInitialAssignment(id).toStdString();
  if (!sampledFieldID.empty()) {
    const auto *geom = getOrCreateGeometry(sbmlModel);
    const auto *sf = geom->getSampledField(sampledFieldID);
    // use string instead of vector of doubles overload to avoid libsbml issue
    // with stringtreams & subnormal doubles on macos:
    // https://github.com/spatial-model-editor/spatial-model-editor/issues/465
    std::stringstream ss{sf->getSamples()};
    double val;
    while (ss >> val || !ss.eof()) {
      if (ss.fail() && std::fpclassify(val) == FP_SUBNORMAL) {
        // subnormal doubles set fail bit on macos but are otherwise correctly
        // parsed
        ss.clear();
      }
      array.push_back(val);
    }
  }
  SPDLOG_INFO("returning array of size {}", array.size());
  return array;
}

static std::string getOrCreateDiffusionConstantUnit(libsbml::Model *model) {
  auto *l = model->getUnitDefinition(model->getLengthUnits());
  auto *t = model->getUnitDefinition(model->getTimeUnits());
  std::unique_ptr<libsbml::UnitDefinition> l2{
      libsbml::UnitDefinition::combine(l, l)};
  std::unique_ptr<libsbml::UnitDefinition> coeffUnits{
      libsbml::UnitDefinition::divide(l2.get(), t)};
  for (unsigned i = 0; i < model->getNumUnitDefinitions(); ++i) {
    if (const auto *unitDef = model->getUnitDefinition(i);
        libsbml::UnitDefinition::areEquivalent(coeffUnits.get(), unitDef)) {
      SPDLOG_INFO("Found existing Diffusion Coefficient unit");
      SPDLOG_INFO("  - {}", unitDef->getId());
      SPDLOG_INFO("  - {}", libsbml::UnitDefinition::printUnits(unitDef, true));
      return unitDef->getId();
    }
  }
  SPDLOG_INFO("Creating Diffusion Coefficient units");
  std::string newId{"diffusion_constant_units"};
  // ensure it is unique among unitdef Ids
  // note: they are in a different namespace to other SBML objects
  while (model->getUnitDefinition(newId) != nullptr) {
    newId.append("_");
    SPDLOG_DEBUG("  -> {}", newId);
  }
  SPDLOG_DEBUG("creating UnitDefinition {}", newId);
  coeffUnits->setId(newId);
  coeffUnits->setName(newId);
  SPDLOG_INFO("  - {}", coeffUnits->getId());
  SPDLOG_INFO("  - {}",
              libsbml::UnitDefinition::printUnits(coeffUnits.get(), true));
  model->addUnitDefinition(coeffUnits.get());
  return newId;
}

static libsbml::Parameter *
getDiffusionConstantParameter(libsbml::Model *model, const QString &speciesId) {
  for (unsigned i = 0; i < model->getNumParameters(); ++i) {
    auto *param = model->getParameter(i);
    if (auto *spp = dynamic_cast<libsbml::SpatialParameterPlugin *>(
            param->getPlugin("spatial"));
        (spp != nullptr) && spp->isSetDiffusionCoefficient() &&
        (spp->getDiffusionCoefficient()->getVariable() ==
         speciesId.toStdString())) {
      SPDLOG_INFO("  - found existing diffusion constant: {}", param->getId());
      return param;
    }
  }
  return nullptr;
}

static libsbml::Parameter *
getOrCreateDiffusionConstantParameter(libsbml::Model *model,
                                      const QString &speciesId) {
  auto *param = getDiffusionConstantParameter(model, speciesId);
  if (param != nullptr) {
    return param;
  }
  // not found: create a new one
  std::string units = getOrCreateDiffusionConstantUnit(model);
  param = model->createParameter();
  std::string id = speciesId.toStdString() + "_diffusionConstant";
  while (!isSIdAvailable(id, model)) {
    id.append("_");
  }
  param->setId(id);
  param->setValue(1.0);
  param->setConstant(true);
  param->setUnits(units);
  auto *spp = dynamic_cast<libsbml::SpatialParameterPlugin *>(
      param->getPlugin("spatial"));
  auto *diffCoeff = spp->createDiffusionCoefficient();
  diffCoeff->setVariable(speciesId.toStdString());
  diffCoeff->setType(libsbml::DiffusionKind_t::SPATIAL_DIFFUSIONKIND_ISOTROPIC);
  SPDLOG_INFO("  - created new diffusion constant: {} = {}", param->getId(),
              param->getValue());
  return param;
}

ModelSpecies::ModelSpecies() = default;

ModelSpecies::ModelSpecies(libsbml::Model *model,
                           const ModelCompartments *compartments,
                           const ModelGeometry *geometry,
                           const ModelParameters *parameters,
                           ModelReactions *reactions,
                           simulate::SimulationData *data, Settings *annotation)
    : ids{importIds(model)}, names{importNamesAndMakeUnique(model)},
      compartmentIds{importCompartmentIds(model)}, sbmlModel{model},
      modelCompartments{compartments}, modelGeometry{geometry},
      modelParameters{parameters}, modelReactions{reactions},
      simulationData{data}, sbmlAnnotation{annotation} {
  makeInitialConcentrationsValid(model);
  for (int i = 0; i < ids.size(); ++i) {
    const auto &id = ids[i];
    QRgb colour{utils::indexedColours()[static_cast<std::size_t>(i)].rgb()};
    if (auto iter{sbmlAnnotation->speciesColours.find(id.toStdString())};
        iter != sbmlAnnotation->speciesColours.end()) {
      colour = iter->second;
    } else {
      sbmlAnnotation->speciesColours[id.toStdString()] = colour;
    }
    auto &field =
        fields.emplace_back(compartments->getCompartment(compartmentIds[i]),
                            ids[i].toStdString(), 1.0, colour);
    const auto *spec = sbmlModel->getSpecies(id.toStdString());
    fields[static_cast<std::size_t>(i)].setUniformConcentration(
        spec->getInitialConcentration());
    // if sampled field or analytic are present, they override the above
    if (auto sf = getSampledFieldInitialAssignment(id); !sf.isEmpty()) {
      auto arr = getSampledFieldConcentrationFromSBML(id);
      fields[static_cast<std::size_t>(i)].importConcentration(arr);
    } else if (auto expr = getAnalyticConcentration(id); !expr.isEmpty()) {
      setFieldConcAnalytic(fields[static_cast<std::size_t>(i)],
                           expr.toStdString());
    }
    const auto *ssp = static_cast<const libsbml::SpatialSpeciesPlugin *>(
        spec->getPlugin("spatial"));
    field.setIsSpatial(ssp->getIsSpatial());
    const auto *param = getOrCreateDiffusionConstantParameter(sbmlModel, id);
    field.setDiffusionConstant(param->getValue());
  }
}

QString ModelSpecies::add(const QString &name, const QString &compartmentId) {
  hasUnsavedChanges = true;
  SPDLOG_INFO("Clearing simulation data");
  simulationData->clear();
  QString newName = name;
  QString compName = modelCompartments->getName(compartmentId);
  while (names.contains(newName)) {
    newName.append("_");
    newName.append(compName);
  }
  SPDLOG_INFO("Adding new species");
  auto *spec = sbmlModel->createSpecies();
  SPDLOG_INFO("  - name: {}", newName.toStdString());
  spec->setName(newName.toStdString());
  names.push_back(newName);
  auto id = nameToUniqueSId(newName, sbmlModel);
  std::string sId{id.toStdString()};
  SPDLOG_INFO("  - id: {}", sId);
  spec->setId(sId);
  ids.push_back(id);
  SPDLOG_INFO("  - compartment: {}", compartmentId.toStdString());
  spec->setCompartment(compartmentId.toStdString());
  compartmentIds.push_back(compartmentId);
  spec->setHasOnlySubstanceUnits(false);
  spec->setBoundaryCondition(false);
  spec->setConstant(false);
  // set default colour
  auto colour{
      utils::indexedColours()[static_cast<std::size_t>(ids.size() - 1)].rgb()};
  fields.emplace_back(modelCompartments->getCompartment(compartmentId), sId,
                      1.0, colour);
  sbmlAnnotation->speciesColours[sId] = colour;
  setIsSpatial(id, true);
  setDiffusionConstant(id, 1.0);
  setInitialConcentration(id, 0.0);
  return newName;
}

void ModelSpecies::remove(const QString &id) {
  std::string sId = id.toStdString();
  SPDLOG_INFO("Removing species {}", sId);
  auto i = ids.indexOf(id);
  if (i < 0) {
    SPDLOG_WARN("  - species {} not found in ids", sId);
    return;
  }
  SPDLOG_INFO("Clearing simulation data");
  simulationData->clear();
  hasUnsavedChanges = true;
  std::unique_ptr<libsbml::Species> spec(sbmlModel->removeSpecies(sId));
  if (spec == nullptr) {
    SPDLOG_WARN("  - species {} not found in sbml model", sId);
    return;
  }
  // remove species from species list
  ids.removeAt(i);
  names.removeAt(i);
  sbmlAnnotation->speciesColours.erase(sId);
  compartmentIds.removeAt(i);
  removeInitialAssignment(id);
  fields.erase(fields.begin() +
               static_cast<decltype(fields)::difference_type>(i));
  modelReactions->removeAllInvolvingSpecies(id);
  if (auto *param = getDiffusionConstantParameter(sbmlModel, id);
      param != nullptr) {
    SPDLOG_INFO("Removing diffusion constant parameter '{}'", param->getId());
    param->removeFromParentAndDelete();
  }
  SPDLOG_INFO("  - species {} removed", spec->getId());
}

QString ModelSpecies::setName(const QString &id, const QString &name) {
  auto i = ids.indexOf(id);
  if (i < 0) {
    return {};
  }
  hasUnsavedChanges = true;
  QString uniqueName = name;
  while (names.contains(uniqueName)) {
    uniqueName.append("_");
  }
  names[i] = uniqueName;
  std::string sId{id.toStdString()};
  std::string sName{uniqueName.toStdString()};
  auto *species = sbmlModel->getSpecies(sId);
  SPDLOG_INFO("sId '{}' : name -> '{}'", sId, sName);
  species->setName(sName);
  return uniqueName;
}

QString ModelSpecies::getName(const QString &id) const {
  auto i = ids.indexOf(id);
  if (i < 0) {
    return {};
  }
  return names[i];
}

void ModelSpecies::updateCompartmentGeometry(const QString &compartmentId) {
  hasUnsavedChanges = true;
  for (const auto &id : ids) {
    if (getCompartment(id) == compartmentId) {
      setCompartment(id, compartmentId);
    }
  }
}

void ModelSpecies::setCompartment(const QString &id,
                                  const QString &compartmentId) {
  std::string sId = id.toStdString();
  std::string compSId = compartmentId.toStdString();
  if (sbmlModel->getCompartment(compSId) == nullptr) {
    SPDLOG_WARN("Compartment '{}' not found", compSId);
    return;
  }
  auto *spec = sbmlModel->getSpecies(sId);
  if (spec == nullptr) {
    SPDLOG_WARN("Species '{}' not found", sId);
    return;
  }
  SPDLOG_INFO("Clearing simulation data");
  simulationData->clear();
  hasUnsavedChanges = true;
  spec->setCompartment(compSId);
  auto i = ids.indexOf(id);
  fields[static_cast<std::size_t>(i)].setCompartment(
      modelCompartments->getCompartment(compartmentId));
  compartmentIds[i] = compartmentId;
  setInitialConcentration(id, spec->getInitialConcentration());
  // if sampled field or analytic are present, they override the above
  if (auto sf = getSampledFieldInitialAssignment(id); !sf.isEmpty()) {
    auto arr = getSampledFieldConcentrationFromSBML(id);
    fields[static_cast<std::size_t>(i)].importConcentration(arr);
  } else if (auto expr = getAnalyticConcentration(id); !expr.isEmpty()) {
    setFieldConcAnalytic(fields[static_cast<std::size_t>(i)],
                         expr.toStdString());
  }
}

QString ModelSpecies::getCompartment(const QString &id) const {
  auto i = ids.indexOf(id);
  if (i < 0) {
    return {};
  }
  return compartmentIds[i];
}

QStringList ModelSpecies::getIds(const QString &compartmentId) const {
  QStringList list;
  for (int i = 0; i < ids.size(); ++i) {
    if (compartmentIds[i] == compartmentId) {
      list.push_back(ids[i]);
    }
  }
  return list;
}

QStringList ModelSpecies::getNames(const QString &compartmentId) const {
  QStringList list;
  for (int i = 0; i < ids.size(); ++i) {
    if (compartmentIds[i] == compartmentId) {
      list.push_back(names[i]);
    }
  }
  return list;
}

void ModelSpecies::setIsSpatial(const QString &id, bool isSpatial) {
  auto i = ids.indexOf(id);
  fields[static_cast<std::size_t>(i)].setIsSpatial(isSpatial);
  std::string sId = id.toStdString();
  auto *spec = sbmlModel->getSpecies(sId);
  if (spec == nullptr) {
    SPDLOG_ERROR("Failed to get species {}", sId);
    return;
  }
  auto *ssp =
      static_cast<libsbml::SpatialSpeciesPlugin *>(spec->getPlugin("spatial"));
  if (ssp == nullptr) {
    SPDLOG_ERROR("Failed to get SpatialSpeciesPlugin for species {}", sId);
    return;
  }
  hasUnsavedChanges = true;
  ssp->setIsSpatial(isSpatial);
  if (isSpatial) {
    // for now spatial species cannot be constant
    setIsConstant(id, false);
  } else {
    removeInitialAssignment(id);
    setDiffusionConstant(id, 0.0);
  }
}

bool ModelSpecies::getIsSpatial(const QString &id) const {
  auto i = ids.indexOf(id);
  return fields[static_cast<std::size_t>(i)].getIsSpatial();
}

void ModelSpecies::setDiffusionConstant(const QString &id,
                                        double diffusionConstant) {
  hasUnsavedChanges = true;
  auto *param = getOrCreateDiffusionConstantParameter(sbmlModel, id);
  param->setValue(diffusionConstant);
  auto i = ids.indexOf(id);
  fields[static_cast<std::size_t>(i)].setDiffusionConstant(diffusionConstant);
}

double ModelSpecies::getDiffusionConstant(const QString &id) const {
  auto i = ids.indexOf(id);
  return fields[static_cast<std::size_t>(i)].getDiffusionConstant();
}

void ModelSpecies::setInitialConcentration(const QString &id,
                                           double concentration) {
  hasUnsavedChanges = true;
  std::string sId = id.toStdString();
  removeInitialAssignment(id);
  sbmlModel->getSpecies(sId)->setInitialConcentration(concentration);
  auto i = ids.indexOf(id);
  fields[static_cast<std::size_t>(i)].setUniformConcentration(concentration);
}

double ModelSpecies::getInitialConcentration(const QString &id) const {
  return sbmlModel->getSpecies(id.toStdString())->getInitialConcentration();
}

void ModelSpecies::setAnalyticConcentration(const QString &id,
                                            const QString &analyticExpression) {
  std::string sId{id.toStdString()};
  SPDLOG_INFO("speciesID: {}", sId);
  SPDLOG_INFO("  - expression: {}", analyticExpression.toStdString());
  std::unique_ptr<libsbml::ASTNode> argAST(
      libsbml::SBML_parseL3Formula(analyticExpression.toStdString().c_str()));
  if (argAST == nullptr) {
    SPDLOG_ERROR("  - libSBML failed to parse expression");
    return;
  }
  hasUnsavedChanges = true;
  removeInitialAssignment(id);
  auto *asgn = sbmlModel->createInitialAssignment();
  asgn->setSymbol(sId);
  asgn->setId(sId + "_initialConcentration");
  SPDLOG_INFO("  - creating new assignment: {}", asgn->getId());
  asgn->setMath(argAST.get());
  auto i = ids.indexOf(id);
  setFieldConcAnalytic(fields[static_cast<std::size_t>(i)],
                       analyticExpression.toStdString());
}

QString ModelSpecies::getAnalyticConcentration(const QString &id) const {
  auto sf = getSampledFieldInitialAssignment(id);
  if (!sf.isEmpty()) {
    return {};
  }
  const auto *asgn = sbmlModel->getInitialAssignmentBySymbol(id.toStdString());
  if (asgn != nullptr) {
    return mathASTtoString(asgn->getMath()).c_str();
  }
  return {};
}

void ModelSpecies::setSampledFieldConcentration(
    const QString &id, const std::vector<double> &concentrationArray) {
  hasUnsavedChanges = true;
  std::string sId = id.toStdString();
  SPDLOG_INFO("speciesID: {}", sId);
  removeInitialAssignment(id);
  // sampled field
  auto *geom = getOrCreateGeometry(sbmlModel);
  auto *sf = geom->createSampledField();
  std::string sfId = id.toStdString() + "_initialConcentration";
  while (!isSpatialIdAvailable(sfId, geom)) {
    sfId.append("_");
  }
  sf->setId(sfId);
  SPDLOG_INFO("  - creating SampledField: {}", sf->getId());
  sf->setSamples(concentrationArray);
  sf->setNumSamples1(modelGeometry->getImage().width());
  sf->setNumSamples2(modelGeometry->getImage().height());
  sf->setSamplesLength(modelGeometry->getImage().width() *
                       modelGeometry->getImage().height());
  SPDLOG_INFO("  - set samples to {}x{} array", sf->getNumSamples1(),
              sf->getNumSamples2());
  sf->setDataType(libsbml::DataKind_t::SPATIAL_DATAKIND_DOUBLE);
  sf->setInterpolationType(
      libsbml::InterpolationKind_t::SPATIAL_INTERPOLATIONKIND_LINEAR);
  sf->setCompression(
      libsbml::CompressionKind_t::SPATIAL_COMPRESSIONKIND_UNCOMPRESSED);
  // create SBML parameter with spatial ref to sampled field
  auto *param = sbmlModel->createParameter();
  auto paramId = id.toStdString() + "_initialConcentration";
  while (!isSIdAvailable(paramId, sbmlModel)) {
    paramId.append("_");
  }
  param->setId(paramId);
  param->setConstant(true);
  param->setUnits(sbmlModel->getSubstanceUnits());
  SPDLOG_INFO("  - creating Parameter: {}", param->getId());
  auto *spp = static_cast<libsbml::SpatialParameterPlugin *>(
      param->getPlugin("spatial"));
  auto *ssr = spp->createSpatialSymbolReference();
  ssr->setSpatialRef(sf->getId());
  SPDLOG_INFO("  - with spatialSymbolReference: {}", ssr->getSpatialRef());
  auto *asgn = sbmlModel->createInitialAssignment();
  asgn->setSymbol(sId);
  std::unique_ptr<libsbml::ASTNode> argAST(
      libsbml::SBML_parseL3Formula(param->getId().c_str()));
  asgn->setMath(argAST.get());
  SPDLOG_INFO("  - creating initialAssignment: {}", asgn->getMath()->getName());
  auto i = ids.indexOf(id);
  fields[static_cast<std::size_t>(i)].importConcentration(concentrationArray);
}

std::vector<double>
ModelSpecies::getSampledFieldConcentration(const QString &id) const {
  auto i = ids.indexOf(id);
  return fields[static_cast<std::size_t>(i)].getConcentrationImageArray();
}

QImage ModelSpecies::getConcentrationImage(const QString &id) const {
  auto i = ids.indexOf(id);
  return fields[static_cast<std::size_t>(i)].getConcentrationImage();
}

void ModelSpecies::setColour(const QString &id, QRgb colour) {
  auto i = ids.indexOf(id);
  if (i < 0) {
    return;
  }
  hasUnsavedChanges = true;
  fields[static_cast<std::size_t>(i)].setColour(colour);
  sbmlAnnotation->speciesColours[id.toStdString()] = colour;
}

QRgb ModelSpecies::getColour(const QString &id) const {
  auto i = ids.indexOf(id);
  if (i < 0) {
    return 0;
  }
  return fields[static_cast<std::size_t>(i)].getColour();
}

void ModelSpecies::setIsConstant(const QString &id, bool constant) {
  hasUnsavedChanges = true;
  auto *spec = sbmlModel->getSpecies(id.toStdString());
  spec->setConstant(constant);
  if (constant) {
    // for now: constant species must be non-spatial
    setIsSpatial(id, false);
  }
  // todo: think about how to deal with boundaryCondition properly
  // for now, just set it to false here
  // i.e. this species cannot be a product or reactant
  spec->setBoundaryCondition(false);
}

bool ModelSpecies::getIsConstant(const QString &id) const {
  const auto *spec = sbmlModel->getSpecies(id.toStdString());
  return getIsSpeciesConstant(spec);
}

bool ModelSpecies::isReactive(const QString &id) const {
  // true if this species should have a PDE generated for it
  // by the Reactions that involve it
  const auto *spec = sbmlModel->getSpecies(id.toStdString());
  if (spec == nullptr) {
    return false;
  }
  return !(spec->isSetConstant() && spec->getConstant()) &&
         !(spec->isSetBoundaryCondition() && spec->getBoundaryCondition());
}

void ModelSpecies::removeInitialAssignments() {
  hasUnsavedChanges = true;
  for (const auto &id : ids) {
    removeInitialAssignment(id);
  }
}

QString
ModelSpecies::getSampledFieldInitialAssignment(const QString &id) const {
  // look for existing initialAssignment to a sampledField
  if (const auto *asgn =
          sbmlModel->getInitialAssignmentBySymbol(id.toStdString());
      asgn != nullptr && asgn->getMath()->isName()) {
    std::string paramID = asgn->getMath()->getName();
    SPDLOG_INFO("  - found initialAssignment: {}", paramID);
    if (const auto *param = sbmlModel->getParameter(paramID);
        param != nullptr) {
      if (const auto *spp =
              dynamic_cast<const libsbml::SpatialParameterPlugin *>(
                  param->getPlugin("spatial"));
          spp != nullptr) {
        if (const auto *ssr = spp->getSpatialSymbolReference();
            ssr != nullptr) {
          const auto &ref = ssr->getSpatialRef();
          SPDLOG_INFO("  - found spatialSymbolReference: {}", ref);
          if (const auto *geom = getOrCreateGeometry(sbmlModel);
              geom->getSampledField(ref) != nullptr) {
            SPDLOG_INFO("  - this is a reference to a SampledField");
            return ref.c_str();
          }
        }
      }
    }
  }
  return {};
}

geometry::Field *ModelSpecies::getField(const QString &id) {
  auto i = ids.indexOf(id);
  if (i < 0) {
    return nullptr;
  }
  return &fields[static_cast<std::size_t>(i)];
}

const geometry::Field *ModelSpecies::getField(const QString &id) const {
  auto i = ids.indexOf(id);
  if (i < 0) {
    return nullptr;
  }
  return &fields[static_cast<std::size_t>(i)];
}

bool ModelSpecies::getHasUnsavedChanges() const { return hasUnsavedChanges; }

void ModelSpecies::setHasUnsavedChanges(bool unsavedChanges) {
  hasUnsavedChanges = unsavedChanges;
}

} // namespace model

} // namespace sme
