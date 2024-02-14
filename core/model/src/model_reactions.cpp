#include "sme/model_reactions.hpp"
#include "id.hpp"
#include "sbml_math.hpp"
#include "sme/geometry.hpp"
#include "sme/logger.hpp"
#include "sme/model_compartments.hpp"
#include "sme/model_membranes.hpp"
#include "sme/simple_symbolic.hpp"
#include <QString>
#include <algorithm>
#include <memory>
#include <sbml/SBMLTypes.h>
#include <sbml/extension/SBMLDocumentPlugin.h>
#include <sbml/packages/spatial/common/SpatialExtensionTypes.h>
#include <sbml/packages/spatial/extension/SpatialExtension.h>
#include <set>

namespace sme::model {

static QStringList importIds(const libsbml::Model *model) {
  QStringList ids;
  for (unsigned int i = 0; i < model->getNumReactions(); ++i) {
    const auto *reaction{model->getReaction(i)};
    ids.emplace_back(reaction->getId().c_str());
  }
  return ids;
}

static QStringList importNamesAndMakeUnique(libsbml::Model *model) {
  QStringList names;
  for (unsigned int i = 0; i < model->getNumReactions(); ++i) {
    auto *reaction{model->getReaction(i)};
    const auto &sId{reaction->getId()};
    if (reaction->getName().empty()) {
      SPDLOG_INFO("Reaction '{}' has no Name, using sId", sId);
      reaction->setName(sId);
    }
    auto name{makeUnique(reaction->getName().c_str(), names)};
    reaction->setName(name.toStdString());
    names.emplace_back(name);
    // also set isLocal to true for all reactions
    auto *srp{static_cast<libsbml::SpatialReactionPlugin *>(
        reaction->getPlugin("spatial"))};
    srp->setIsLocal(true);
  }
  return names;
}

static QVector<QStringList> importParameterIds(libsbml::Model *model) {
  QVector<QStringList> paramIds;
  for (unsigned int i = 0; i < model->getNumReactions(); ++i) {
    auto *reac{model->getReaction(i)};
    auto *kin{reac->getKineticLaw()};
    auto &ids{paramIds.emplace_back()};
    for (unsigned int j = 0; j < kin->getNumLocalParameters(); ++j) {
      auto *param{kin->getLocalParameter(j)};
      const auto &sId{param->getId()};
      if (param->getName().empty()) {
        SPDLOG_INFO("ReactionParameter '{}' has no Name, using sId", sId);
        param->setName(sId);
      }
      ids.emplace_back(sId.c_str());
    }
  }
  return paramIds;
}

static std::string
inferReactionCompartment(const libsbml::Reaction *reac,
                         const std::vector<geometry::Membrane> &membranes) {
  const auto *model{reac->getModel()};
  std::set<std::string, std::less<>> possibleCompartments;
  if (reac->isSetCompartment()) {
    possibleCompartments.insert(reac->getCompartment());
  }
  for (unsigned int k = 0; k < reac->getNumProducts(); ++k) {
    const auto &sId = reac->getProduct(k)->getSpecies();
    const auto &compId = model->getSpecies(sId)->getCompartment();
    possibleCompartments.insert(compId);
  }
  for (unsigned int k = 0; k < reac->getNumReactants(); ++k) {
    const auto &sId = reac->getReactant(k)->getSpecies();
    const auto &compId = model->getSpecies(sId)->getCompartment();
    possibleCompartments.insert(compId);
  }
  for (unsigned int k = 0; k < reac->getNumModifiers(); ++k) {
    const auto &sId = reac->getModifier(k)->getSpecies();
    const auto &compId = model->getSpecies(sId)->getCompartment();
    possibleCompartments.insert(compId);
  }
  if (possibleCompartments.size() == 1) {
    SPDLOG_INFO("Reaction involves species from a single compartment:");
    SPDLOG_INFO("  -> Compartment '{}'", *possibleCompartments.begin());
    return *possibleCompartments.begin();
  } else if (possibleCompartments.size() == 2) {
    SPDLOG_INFO("Reaction involves species from two compartments:");
    SPDLOG_INFO("  - '{}'", *possibleCompartments.begin());
    SPDLOG_INFO("  - '{}'", *possibleCompartments.rbegin());
    for (const auto &membrane : membranes) {
      const auto &cA{membrane.getCompartmentA()->getId()};
      const auto &cB{membrane.getCompartmentB()->getId()};
      if (possibleCompartments.contains(cA) &&
          possibleCompartments.contains(cB)) {
        SPDLOG_INFO("  -> Membrane '{}'", membrane.getId());
        return membrane.getId();
      }
    }
  } else {
    SPDLOG_WARN("No valid compartment for reaction");
    if (model->getNumCompartments() > 0) {
      const auto &compId{model->getCompartment(0)->getId()};
      SPDLOG_WARN("Using first compartment we find: {}", compId);
      return compId;
    }
  }
  return {};
}

static ReactionRescaling getReactionRescaling(
    const libsbml::Reaction *reaction, const std::string &divisor,
    const std::vector<std::pair<std::string, double>> &constants) {
  ReactionRescaling reactionRescaling;
  reactionRescaling.id = reaction->getId();
  reactionRescaling.reactionName = reaction->getName().c_str();
  const auto *model{reaction->getModel()};
  const auto &compartmentId{reaction->getCompartment()};
  reactionRescaling.reactionLocation =
      model->getCompartment(compartmentId)->getName().c_str();
  SPDLOG_INFO("  {} ('{}')", reactionRescaling.id,
              reactionRescaling.reactionName.toStdString());
  const auto *kineticLaw{reaction->getKineticLaw()};
  auto expr{mathASTtoString(kineticLaw->getMath())};
  reactionRescaling.originalExpression = expr.c_str();
  SPDLOG_INFO("  - {}", expr);
  expr = common::SimpleSymbolic::divide(expr, divisor);
  SPDLOG_INFO("  --> {}", expr);
  expr = common::SimpleSymbolic::substitute(expr, constants);
  SPDLOG_INFO("  --> {}", expr);
  reactionRescaling.rescaledExpression = expr.c_str();
  return reactionRescaling;
}

static bool reactionInvolvesSpecies(const libsbml::Reaction *reac,
                                    const std::string &speciesId) {
  for (unsigned i = 0; i < reac->getNumProducts(); ++i) {
    if (reac->getProduct(i)->getSpecies() == speciesId) {
      return true;
    }
  }
  for (unsigned i = 0; i < reac->getNumReactants(); ++i) {
    if (reac->getReactant(i)->getSpecies() == speciesId) {
      return true;
    }
  }
  for (unsigned i = 0; i < reac->getNumModifiers(); ++i) {
    if (reac->getModifier(i)->getSpecies() == speciesId) {
      return true;
    }
  }
  return false;
}

static std::set<std::string, std::less<>>
getValidSpeciesIds(const libsbml::Reaction *reaction,
                   const std::vector<geometry::Membrane> &membranes) {
  std::set<std::string, std::less<>> validSpeciesIds{};
  const auto *model{reaction->getModel()};
  std::string compA{reaction->getCompartment()};
  std::string compB{};
  if (auto iter{std::ranges::find_if(membranes,
                                     [&compA](const auto &membrane) {
                                       return membrane.getId() == compA;
                                     })};
      iter != membranes.cend()) {
    // reaction location is a membrane: species can come from either
    // neighbouring compartment
    compA = iter->getCompartmentA()->getId();
    compB = iter->getCompartmentB()->getId();
  }
  for (unsigned i = 0; i < model->getNumSpecies(); ++i) {
    const auto *species{model->getSpecies(i)};
    if (const auto &compartment{species->getCompartment()};
        compartment == compA || compartment == compB) {
      validSpeciesIds.insert(species->getId());
    }
  }
  return validSpeciesIds;
}

static std::set<std::string, std::less<>> getPossibleModifierSpeciesIds(
    const libsbml::Reaction *reaction,
    const std::vector<geometry::Membrane> &membranes) {
  auto modifierSpecies{getValidSpeciesIds(reaction, membranes)};
  for (unsigned i = 0; i < reaction->getNumProducts(); ++i) {
    modifierSpecies.erase(reaction->getProduct(i)->getSpecies());
  }
  for (unsigned i = 0; i < reaction->getNumReactants(); ++i) {
    modifierSpecies.erase(reaction->getReactant(i)->getSpecies());
  }
  return modifierSpecies;
}

static void
removeInvalidSpecies(libsbml::Reaction *reaction,
                     const std::vector<geometry::Membrane> &membranes) {
  auto validSpeciesIds{getValidSpeciesIds(reaction, membranes)};
  std::vector<std::string> productsToRemove;
  for (unsigned i = 0; i < reaction->getNumProducts(); ++i) {
    if (auto sId{reaction->getProduct(i)->getSpecies()};
        !validSpeciesIds.contains(sId)) {
      productsToRemove.emplace_back(sId);
    }
  }
  for (const auto &sId : productsToRemove) {
    std::unique_ptr<libsbml::SpeciesReference> rmProduct{
        reaction->removeProduct(sId)};
    SPDLOG_INFO("removing invalid product '{}' from reaction '{}'",
                rmProduct->getSpecies(), reaction->getId());
  }
  std::vector<std::string> reactantsToRemove;
  for (unsigned i = 0; i < reaction->getNumReactants(); ++i) {
    if (auto sId{reaction->getReactant(i)->getSpecies()};
        !validSpeciesIds.contains(sId)) {
      reactantsToRemove.emplace_back(sId);
    }
  }
  for (const auto &sId : reactantsToRemove) {
    std::unique_ptr<libsbml::SpeciesReference> rmReactant{
        reaction->removeReactant(sId)};
    SPDLOG_INFO("removing invalid reactant '{}' from reaction '{}'",
                rmReactant->getSpecies(), reaction->getId());
  }
}

ModelReactions::ModelReactions() = default;

ModelReactions::ModelReactions(libsbml::Model *model,
                               const ModelCompartments *compartments,
                               const ModelMembranes *membranes,
                               bool isNonSpatialModel)
    : ids{importIds(model)}, names{importNamesAndMakeUnique(model)},
      parameterIds{importParameterIds(model)}, sbmlModel{model},
      modelCompartments{compartments}, modelMembranes{membranes},
      isIncompleteODEImport{isNonSpatialModel} {
  if (!isNonSpatialModel) {
    // for a non-spatial model, we'll assign missing reaction locations after
    // the geometry is imported - but if we import a spatial model with
    // reactions without a location, should fix that straight away:
    makeReactionLocationsValid();
  }
}

void ModelReactions::makeReactionLocationsValid() {
  if (sbmlModel == nullptr) {
    return;
  }
  hasUnsavedChanges = true;
  const auto &membranes{modelMembranes->getMembranes()};
  for (unsigned int i = 0; i < sbmlModel->getNumReactions(); ++i) {
    auto *reaction{sbmlModel->getReaction(i)};
    reaction->setFast(false);
    if (const auto *kineticLaw{reaction->getKineticLaw()};
        kineticLaw == nullptr) {
      kineticLaw = reaction->createKineticLaw();
    }
    if (reaction->getCompartment().empty()) {
      // make a best guess for reaction location based on species involved
      SPDLOG_INFO("spatial reaction '{}' compartment not set",
                  reaction->getId());
      reaction->setCompartment(inferReactionCompartment(reaction, membranes));
      // ensure species involved make sense
      removeInvalidSpecies(reaction, membranes);
    }
  }
}

void ModelReactions::applySpatialReactionRescalings(
    const std::vector<ReactionRescaling> &reactionRescalings) {
  for (const auto &reactionRescaling : reactionRescalings) {
    auto *reaction{sbmlModel->getReaction(reactionRescaling.id)};
    if (auto ast{mathStringToAST(
            reactionRescaling.rescaledExpression.toStdString(), sbmlModel)};
        ast == nullptr) {
      SPDLOG_WARN(
          "  - libSBML failed to parse expression: leaving existing math");
    } else {
      reaction->getKineticLaw()->setMath(ast.get());
      SPDLOG_INFO("  - new math: {}",
                  mathASTtoString(reaction->getKineticLaw()->getMath()));
    }
  }
  isIncompleteODEImport = false;
}

std::vector<ReactionRescaling>
ModelReactions::getSpatialReactionRescalings() const {
  std::vector<ReactionRescaling> reactionRescaling;
  reactionRescaling.reserve(
      static_cast<std::size_t>(sbmlModel->getNumReactions()));
  std::vector<std::pair<std::string, double>> compartmentSizes;
  compartmentSizes.reserve(
      static_cast<std::size_t>(sbmlModel->getNumCompartments()));
  for (unsigned i = 0; i < sbmlModel->getNumCompartments(); ++i) {
    const auto *comp{sbmlModel->getCompartment(i)};
    compartmentSizes.emplace_back(comp->getId(), comp->getSize());
  }
  for (unsigned int i = 0; i < sbmlModel->getNumReactions(); ++i) {
    const auto *reaction{sbmlModel->getReaction(i)};
    const auto &compartmentId{reaction->getCompartment()};
    SPDLOG_INFO("Location: {}", compartmentId);
    reactionRescaling.emplace_back(
        getReactionRescaling(reaction, compartmentId, compartmentSizes));
  }
  return reactionRescaling;
}

[[nodiscard]] bool ModelReactions::getIsIncompleteODEImport() const {
  return isIncompleteODEImport;
}

QStringList ModelReactions::getIds(const QString &locationId) const {
  QStringList r;
  r.reserve(ids.size());
  std::string compId = locationId.toStdString();
  for (const auto &id : ids) {
    if (sbmlModel->getReaction(id.toStdString())->getCompartment() == compId) {
      r.emplace_back(id);
    }
  }
  return r;
}

[[nodiscard]] QStringList
ModelReactions::getIds(const ReactionLocation &reactionLocation) const {
  QStringList r;
  if (reactionLocation.type != ReactionLocation::Type::Invalid) {
    return getIds(reactionLocation.id);
  }
  QStringList validLocations{modelCompartments->getIds()};
  validLocations << modelMembranes->getIds();
  for (const auto &id : ids) {
    const auto &locationId{
        sbmlModel->getReaction(id.toStdString())->getCompartment()};
    if (!validLocations.contains(locationId.c_str())) {
      r.emplace_back(id);
    }
  }
  return r;
}

[[nodiscard]] std::vector<ReactionLocation>
ModelReactions::getReactionLocations() const {
  std::vector<ReactionLocation> reactionLocations;
  const auto &compartmentIds{modelCompartments->getIds()};
  const auto &membraneIds{modelMembranes->getIds()};
  reactionLocations.reserve(
      static_cast<std::size_t>(compartmentIds.size() + membraneIds.size()));
  for (const auto &compartmentId : compartmentIds) {
    reactionLocations.push_back({compartmentId,
                                 modelCompartments->getName(compartmentId),
                                 ReactionLocation::Type::Compartment});
  }
  for (const auto &membraneId : membraneIds) {
    reactionLocations.push_back({membraneId,
                                 modelMembranes->getName(membraneId),
                                 ReactionLocation::Type::Membrane});
  }
  reactionLocations.push_back(
      {"invalid", "Invalid Location", ReactionLocation::Type::Invalid});
  return reactionLocations;
}

QString ModelReactions::add(const QString &name, const QString &locationId,
                            const QString &rateExpression) {
  hasUnsavedChanges = true;
  auto newName = makeUnique(name, names);
  SPDLOG_INFO("Adding new reaction");
  auto *reaction{sbmlModel->createReaction()};
  SPDLOG_INFO("  - name: {}", newName.toStdString());
  reaction->setName(newName.toStdString());
  names.emplace_back(newName);
  auto id = nameToUniqueSId(newName, sbmlModel);
  std::string sId{id.toStdString()};
  SPDLOG_INFO("  - id: {}", sId);
  reaction->setId(sId);
  ids.emplace_back(id);
  parameterIds.emplace_back();
  reaction->setFast(false);
  reaction->setCompartment(locationId.toStdString());
  reaction->setReversible(true);
  auto *srp = static_cast<libsbml::SpatialReactionPlugin *>(
      reaction->getPlugin("spatial"));
  srp->setIsLocal(true);
  SPDLOG_INFO("  - location: {}", reaction->getCompartment());
  auto *kin = reaction->createKineticLaw();
  kin->setFormula(rateExpression.toStdString());
  return newName;
}

void ModelReactions::remove(const QString &id) {
  auto i{ids.indexOf(id)};
  auto sId{id.toStdString()};
  SPDLOG_INFO("Removing reaction {}", sId);
  std::unique_ptr<libsbml::Reaction> rmReac(sbmlModel->removeReaction(sId));
  if (rmReac == nullptr) {
    SPDLOG_WARN("  - reaction {} not found in SBML", sId);
    return;
  }
  hasUnsavedChanges = true;
  ids.removeAt(i);
  names.removeAt(i);
  parameterIds.removeAt(i);
}

void ModelReactions::removeAllInvolvingSpecies(const QString &speciesId) {
  QStringList reactionIdsToRemove;
  for (unsigned i = 0; i < sbmlModel->getNumReactions(); ++i) {
    const auto *reaction{sbmlModel->getReaction(i)};
    if (reactionInvolvesSpecies(reaction, speciesId.toStdString())) {
      SPDLOG_INFO("  - removing reaction {}", reaction->getId());
      reactionIdsToRemove.emplace_back(reaction->getId().c_str());
    }
  }
  for (const auto &reactionId : reactionIdsToRemove) {
    remove(reactionId);
  }
}

QString ModelReactions::setName(const QString &id, const QString &name) {
  auto i = ids.indexOf(id);
  if (i < 0) {
    return {};
  }
  if (names[i] == name) {
    // no-op: setting name to the same value as it already had
    return name;
  }
  hasUnsavedChanges = true;
  auto uniqueName = makeUnique(name, names);
  names[i] = uniqueName;
  std::string sId{id.toStdString()};
  std::string sName{uniqueName.toStdString()};
  auto *reaction{sbmlModel->getReaction(sId)};
  SPDLOG_INFO("sId '{}' : name -> '{}'", sId, sName);
  reaction->setName(sName);
  return uniqueName;
}

QString ModelReactions::getName(const QString &id) const {
  auto i = ids.indexOf(id);
  if (i < 0) {
    return {};
  }
  return names[i];
}

static QString stoichiometryToString(double stoichiometry) {
  if (stoichiometry == 1.0) {
    return "";
  }
  if (std::floor(stoichiometry) == stoichiometry) {
    return QString::number(static_cast<int>(stoichiometry)) + " ";
  }
  return QString::number(stoichiometry) + " ";
}

static void appendSpeciesToScheme(QString &scheme, const QString &speciesName,
                                  double stoichiometry) {
  if (stoichiometry == 0) {
    return;
  }
  scheme.append(stoichiometryToString(stoichiometry));
  scheme.append(speciesName);
  scheme.append(" + ");
}

QString ModelReactions::getScheme(const QString &id) const {
  const auto *reaction{sbmlModel->getReaction(id.toStdString())};
  if (reaction == nullptr) {
    SPDLOG_WARN("Reaction '{}' not found", id.toStdString());
    return {};
  }
  QString lhs;
  const unsigned numReactants{reaction->getNumReactants()};
  for (unsigned i = 0; i < numReactants; ++i) {
    const auto *r{reaction->getReactant(i)};
    const auto &speciesName{sbmlModel->getSpecies(r->getSpecies())->getName()};
    appendSpeciesToScheme(lhs, speciesName.c_str(), r->getStoichiometry());
  }
  lhs.chop(3);
  QString rhs;
  const unsigned numProducts{reaction->getNumProducts()};
  for (unsigned i = 0; i < numProducts; ++i) {
    const auto *r{reaction->getProduct(i)};
    const auto &speciesName{sbmlModel->getSpecies(r->getSpecies())->getName()};
    appendSpeciesToScheme(rhs, speciesName.c_str(), r->getStoichiometry());
  }
  rhs.chop(3);
  if (lhs.isEmpty() && rhs.isEmpty()) {
    return {};
  }
  return QString("%1 -> %2").arg(lhs, rhs);
}

void ModelReactions::setLocation(const QString &id, const QString &locationId) {
  auto *reaction{sbmlModel->getReaction(id.toStdString())};
  if (reaction == nullptr) {
    SPDLOG_WARN("Reaction '{}' not found", id.toStdString());
    return;
  }
  hasUnsavedChanges = true;
  const auto &membranes{modelMembranes->getMembranes()};
  SPDLOG_INFO("Setting reaction '{}' location to '{}'", id.toStdString(),
              locationId.toStdString());
  reaction->setCompartment(locationId.toStdString());
  removeInvalidSpecies(reaction, membranes);
}

QString ModelReactions::getLocation(const QString &id) const {
  const auto *reaction{sbmlModel->getReaction(id.toStdString())};
  if (reaction == nullptr) {
    return {};
  }
  return reaction->getCompartment().c_str();
}

double ModelReactions::getSpeciesStoichiometry(const QString &id,
                                               const QString &speciesId) const {
  const auto *reaction{sbmlModel->getReaction(id.toStdString())};
  std::string sId{speciesId.toStdString()};
  double stoichiometry{0};
  if (const auto *product{reaction->getProduct(sId)}; product != nullptr) {
    stoichiometry += product->getStoichiometry();
  }
  if (const auto *reactant{reaction->getReactant(sId)}; reactant != nullptr) {
    stoichiometry -= reactant->getStoichiometry();
  }
  return stoichiometry;
}

void ModelReactions::setSpeciesStoichiometry(const QString &id,
                                             const QString &speciesId,
                                             double stoichiometry) {
  hasUnsavedChanges = true;
  auto *reaction{sbmlModel->getReaction(id.toStdString())};
  std::string sId{speciesId.toStdString()};
  const auto *spec{sbmlModel->getSpecies(sId)};
  SPDLOG_TRACE("Reaction '{}', Species '{}', Stoichiometry {}",
               reaction->getId(), spec->getId(), stoichiometry);
  if (std::unique_ptr<libsbml::SpeciesReference> rmProduct(
          reaction->removeProduct(sId));
      rmProduct != nullptr) {
    SPDLOG_TRACE("  - removed Product '{}'", rmProduct->getSpecies());
  }
  if (std::unique_ptr<libsbml::SpeciesReference> rmReactant(
          reaction->removeReactant(sId));
      rmReactant != nullptr) {
    SPDLOG_TRACE("  - removed Reactant '{}'", rmReactant->getSpecies());
  }
  if (std::unique_ptr<libsbml::ModifierSpeciesReference> rmModifier(
          reaction->removeModifier(sId));
      rmModifier != nullptr) {
    SPDLOG_TRACE("  - removed Modifier '{}'", rmModifier->getSpecies());
  }
  if (stoichiometry > 0) {
    reaction->addProduct(spec, stoichiometry);
    SPDLOG_TRACE("  - adding Product '{}' with stoichiometry {}", spec->getId(),
                 stoichiometry);
  }
  if (stoichiometry < 0) {
    reaction->addReactant(spec, -stoichiometry);
    SPDLOG_TRACE("  - adding Reactant '{}' with stoichiometry {}",
                 spec->getId(), -stoichiometry);
  }
  if (stoichiometry == 0 && common::SimpleSymbolic::contains(
                                getRateExpression(id).toStdString(), sId)) {
    reaction->addModifier(spec);
    SPDLOG_TRACE("  - adding Modifier '{}'", spec->getId());
  }
}

QString ModelReactions::getRateExpression(const QString &id) const {
  const auto *reaction{sbmlModel->getReaction(id.toStdString())};
  if (reaction == nullptr) {
    return {};
  }
  const auto *kineticLaw{reaction->getKineticLaw()};
  if (kineticLaw == nullptr) {
    return {};
  }
  return kineticLaw->getFormula().c_str();
}

static libsbml::KineticLaw *getOrCreateKineticLaw(libsbml::Model *model,
                                                  const QString &reactionId) {
  auto *reaction{model->getReaction(reactionId.toStdString())};
  auto *kineticLaw{reaction->getKineticLaw()};
  if (kineticLaw == nullptr) {
    kineticLaw = reaction->createKineticLaw();
  }
  return kineticLaw;
}

void ModelReactions::setRateExpression(const QString &id,
                                       const QString &expression) {
  auto *reaction{sbmlModel->getReaction(id.toStdString())};
  auto *kin{getOrCreateKineticLaw(sbmlModel, id)};
  std::string expr{expression.toStdString()};
  SPDLOG_INFO("  - expr: {}", expr);
  auto exprAST{mathStringToAST(expr, sbmlModel)};
  if (exprAST == nullptr) {
    SPDLOG_ERROR("SBML failed to parse expression: {}",
                 libsbml::SBML_getLastParseL3Error());
    return;
  }
  hasUnsavedChanges = true;
  auto modifierSpecies{
      getPossibleModifierSpeciesIds(reaction, modelMembranes->getMembranes())};
  auto exprSpecies{common::SimpleSymbolic::symbols(expr)};
  reaction->getListOfModifiers()->clear();
  for (const auto &sId : modifierSpecies) {
    if (exprSpecies.find(sId) != exprSpecies.end()) {
      if (const auto *spec{sbmlModel->getSpecies(sId)}; spec != nullptr) {
        SPDLOG_TRACE("  - adding Modifier '{}'", spec->getId());
        reaction->addModifier(spec);
      }
    }
  }
  kin->setMath(exprAST.get());
}

static libsbml::LocalParameter *getLocalParameter(libsbml::Model *model,
                                                  const QString &reactionId,
                                                  const QString &parameterId) {
  auto *reaction{model->getReaction(reactionId.toStdString())};
  if (reaction == nullptr) {
    return nullptr;
  }
  auto *kineticLaw{reaction->getKineticLaw()};
  if (kineticLaw == nullptr) {
    return nullptr;
  }
  return kineticLaw->getLocalParameter(parameterId.toStdString());
}

QStringList ModelReactions::getParameterIds(const QString &id) const {
  auto i = ids.indexOf(id);
  if (i < 0) {
    SPDLOG_ERROR("Reaction '{}' not found", id.toStdString());
    return {};
  }
  return parameterIds[i];
}

QString ModelReactions::setParameterName(const QString &reactionId,
                                         const QString &parameterId,
                                         const QString &name) {
  hasUnsavedChanges = true;
  const auto &pars = parameterIds[ids.indexOf(reactionId)];
  QStringList parNames;
  parNames.reserve(pars.size());
  for (const auto &id : pars) {
    parNames.emplace_back(getParameterName(reactionId, id));
  }
  auto uniqueName = makeUnique(name, parNames);
  std::string sName{uniqueName.toStdString()};
  SPDLOG_INFO("sId '{}' : name -> '{}'", parameterId.toStdString(), sName);
  auto *param = getLocalParameter(sbmlModel, reactionId, parameterId);
  param->setName(sName);
  return uniqueName;
}

QString ModelReactions::getParameterName(const QString &reactionId,
                                         const QString &parameterId) const {
  const auto *param = getLocalParameter(sbmlModel, reactionId, parameterId);
  if (param == nullptr) {
    SPDLOG_ERROR("Parameter '{}' not found in reaction '{}'",
                 parameterId.toStdString(), reactionId.toStdString());
    return {};
  }
  return param->getName().c_str();
}

void ModelReactions::setParameterValue(const QString &reactionId,
                                       const QString &parameterId,
                                       double value) {
  hasUnsavedChanges = true;
  auto *param = getLocalParameter(sbmlModel, reactionId, parameterId);
  param->setValue(value);
}

double ModelReactions::getParameterValue(const QString &reactionId,
                                         const QString &parameterId) const {
  const auto *param = getLocalParameter(sbmlModel, reactionId, parameterId);
  if (param == nullptr) {
    SPDLOG_ERROR("Parameter '{}' not found in reaction '{}'",
                 parameterId.toStdString(), reactionId.toStdString());
    return 0;
  }
  return param->getValue();
}

QString ModelReactions::addParameter(const QString &reactionId,
                                     const QString &name, double value) {
  hasUnsavedChanges = true;
  auto i = ids.indexOf(reactionId);
  auto &pars = parameterIds[i];
  auto uniqueName = makeUnique(name, pars);
  SPDLOG_INFO("Adding new reaction parameter");
  auto *reaction{sbmlModel->getReaction(reactionId.toStdString())};
  auto *kin{reaction->getKineticLaw()};
  SPDLOG_INFO("  - name: {}", uniqueName.toStdString());
  auto *param = kin->createLocalParameter();
  param->setName(uniqueName.toStdString());
  auto id = nameToUniqueSId(uniqueName, sbmlModel);
  std::string sId{id.toStdString()};
  SPDLOG_INFO("  - id: {}", sId);
  param->setId(sId);
  pars.emplace_back(id);
  param->setConstant(true);
  param->setValue(value);
  return id;
}

void ModelReactions::removeParameter(const QString &reactionId,
                                     const QString &id) {
  auto iReac{ids.indexOf(reactionId)};
  if (iReac < 0) {
    SPDLOG_WARN("Reaction '{}' not found", reactionId.toStdString());
    return;
  }
  auto wasRemoved{parameterIds[iReac].removeOne(id)};
  if (!wasRemoved) {
    SPDLOG_WARN("parameter '{}' not found in parameterIds of reaction '{}'",
                id.toStdString(), reactionId.toStdString());
  } else {
    SPDLOG_DEBUG(" removed parameter '{}' from Reaction '{}' parameterIds",
                 id.toStdString(), reactionId.toStdString());
    hasUnsavedChanges = true;
  }
  auto *reaction{sbmlModel->getReaction(reactionId.toStdString())};
  if (reaction == nullptr || !reaction->isSetKineticLaw()) {
    SPDLOG_WARN("reaction '{}' not found in sbml or has no kinetic law",
                reactionId.toStdString());
    return;
  }
  if (std::unique_ptr<libsbml::LocalParameter> rmParam{
          reaction->getKineticLaw()->removeLocalParameter(id.toStdString())};
      rmParam != nullptr) {
    SPDLOG_INFO("  - removed LocalParameter '{}' from Reaction '{}'",
                rmParam->getId(), reaction->getId());
    hasUnsavedChanges = true;
  }
}

bool ModelReactions::dependOnVariable(const QString &variableId) const {
  auto v{variableId.toStdString()};
  return std::ranges::any_of(ids, [&v, this](const auto &id) {
    auto e{getRateExpression(id).toStdString()};
    return common::SimpleSymbolic::contains(e, v);
  });
}

bool ModelReactions::getHasUnsavedChanges() const { return hasUnsavedChanges; }

void ModelReactions::setHasUnsavedChanges(bool unsavedChanges) {
  hasUnsavedChanges = unsavedChanges;
}

} // namespace sme::model
