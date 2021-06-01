#include "model_reactions.hpp"
#include "geometry.hpp"
#include "id.hpp"
#include "logger.hpp"
#include "model_membranes.hpp"
#include "sbml_math.hpp"
#include "symbolic.hpp"
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
    ids.push_back(reaction->getId().c_str());
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
    names.push_back(name);
  }
  return names;
}

static QVector<QStringList> importParameterIds(libsbml::Model *model) {
  QVector<QStringList> paramIds;
  for (unsigned int i = 0; i < model->getNumReactions(); ++i) {
    auto *reac{model->getReaction(i)};
    auto *kin{reac->getKineticLaw()};
    paramIds.push_back({});
    auto &ids{paramIds.back()};
    for (unsigned int j = 0; j < kin->getNumLocalParameters(); ++j) {
      auto *param{kin->getLocalParameter(j)};
      const auto &sId{param->getId()};
      if (param->getName().empty()) {
        SPDLOG_INFO("ReactionParameter '{}' has no Name, using sId", sId);
        param->setName(sId);
      }
      ids.push_back(sId.c_str());
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
      if (possibleCompartments.count(cA) + possibleCompartments.count(cB) ==
          2) {
        SPDLOG_INFO("  -> Membrane '{}'", membrane.getId());
        return membrane.getId();
      }
    }
  }
  return {};
}

static bool divideReactionKineticLaw(libsbml::Reaction *reaction,
                                     const std::string &divisor) {
  const auto *model{reaction->getModel()};
  const auto *kineticLaw{reaction->getKineticLaw()};
  auto expr{mathASTtoString(kineticLaw->getMath())};
  SPDLOG_INFO("  - {}", expr);
  auto newExpr{utils::symbolicDivide(expr, divisor)};
  SPDLOG_INFO("  --> {}", newExpr);
  auto ast{mathStringToAST(newExpr, model)};
  if (ast == nullptr) {
    SPDLOG_WARN(
        "  - libSBML failed to parse expression: leaving existing math");
    return false;
  }
  reaction->getKineticLaw()->setMath(ast.get());
  SPDLOG_INFO("  - new math: {}",
              mathASTtoString(reaction->getKineticLaw()->getMath()));
  return true;
}

static bool
makeReactionSpatial(libsbml::Reaction *reaction,
                    const std::vector<geometry::Membrane> &membranes) {
  auto compartmentId{inferReactionCompartment(reaction, membranes)};
  if (compartmentId.empty()) {
    SPDLOG_INFO("no valid compartment found for reaction {}",
                reaction->getId());
    return false;
  }
  if (auto iter{std::find_if(membranes.cbegin(), membranes.cend(),
                             [compartmentId](const auto &m) {
                               return m.getId() == compartmentId;
                             })};
      iter != membranes.cend()) {
    // membrane reaction
    SPDLOG_INFO("Reaction involves species from two compartments:");
    SPDLOG_INFO("Setting Reaction compartment to membrane: '{}'",
                compartmentId);
    reaction->setCompartment(compartmentId);
    SPDLOG_INFO("  - original rate units: d[amount]/dt");
    SPDLOG_INFO(
        "  -> want spatial membrane reaction: d[amount]/d[membrane area]/dt");
    SPDLOG_INFO("  -> dividing rate by membrane area");
    divideReactionKineticLaw(reaction, compartmentId);
    return true;
  }
  // compartment reaction
  SPDLOG_INFO("Reaction involves species from a single compartment");
  SPDLOG_INFO("Setting Reaction compartment: '{}'", compartmentId);
  reaction->setCompartment(compartmentId);
  SPDLOG_INFO("  - original rate units: d[amount]/dt");
  SPDLOG_INFO("  -> want spatial compartment reaction: d[concentration]/dt");
  SPDLOG_INFO("  -> dividing rate by compartment size");
  divideReactionKineticLaw(reaction, compartmentId);
  return true;
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

static void
removeInvalidSpecies(libsbml::Reaction *reaction,
                     const std::vector<geometry::Membrane> &membranes) {
  std::set<std::string, std::less<>> validSpeciesIds{};
  const auto *model{reaction->getModel()};
  std::string compA{reaction->getCompartment()};
  std::string compB{};
  if (auto iter{std::find_if(membranes.cbegin(), membranes.cend(),
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
  std::vector<std::string> productsToRemove;
  for (unsigned i = 0; i < reaction->getNumProducts(); ++i) {
    if (auto sId{reaction->getProduct(i)->getSpecies()};
        validSpeciesIds.count(sId) == 0) {
      productsToRemove.push_back(sId);
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
        validSpeciesIds.count(sId) == 0) {
      reactantsToRemove.push_back(sId);
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
                               const ModelMembranes *membranes)
    : ids{importIds(model)}, names{importNamesAndMakeUnique(model)},
      parameterIds{importParameterIds(model)}, sbmlModel{model},
      modelMembranes{membranes} {
  makeReactionsSpatial(false);
}

void ModelReactions::makeReactionsSpatial(bool haveValidGeometry) {
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
    auto *srp = static_cast<libsbml::SpatialReactionPlugin *>(
        reaction->getPlugin("spatial"));
    if (srp != nullptr && srp->isSetIsLocal() && srp->getIsLocal() &&
        reaction->getCompartment().empty()) {
      // spatial model, local reaction, but no compartment set, so try to set
      // one
      SPDLOG_INFO("spatial reaction '{}' compartment not set",
                  reaction->getId());
      reaction->setCompartment(inferReactionCompartment(reaction, membranes));
    }
    if (srp != nullptr && !(srp->isSetIsLocal() && srp->getIsLocal())) {
      // spatial model, non-local reaction, i.e. we are importing a
      // non-spatial model, so try to rescale it if we have enough geometry info
      // to do so, and if successful, then set isLocal = true
      if (makeReactionSpatial(reaction, membranes)) {
        SPDLOG_INFO("Setting isLocal=true for reaction {}", reaction->getId());
        srp->setIsLocal(true);
      }
    }
    if (haveValidGeometry && srp != nullptr && srp->isSetIsLocal() &&
        srp->getIsLocal()) {
      // spatial, local reaction with valid geometry: ensure species involved
      // make sense
      removeInvalidSpecies(reaction, membranes);
    }
  }
}

QStringList ModelReactions::getIds(const QString &locationId) const {
  QStringList r;
  r.reserve(ids.size());
  std::string compId = locationId.toStdString();
  for (const auto &id : ids) {
    if (sbmlModel->getReaction(id.toStdString())->getCompartment() == compId) {
      r.push_back(id);
    }
  }
  return r;
}

QString ModelReactions::add(const QString &name, const QString &locationId,
                            const QString &rateExpression) {
  hasUnsavedChanges = true;
  auto newName = makeUnique(name, names);
  SPDLOG_INFO("Adding new reaction");
  auto *reac = sbmlModel->createReaction();
  SPDLOG_INFO("  - name: {}", newName.toStdString());
  reac->setName(newName.toStdString());
  names.push_back(newName);
  auto id = nameToUniqueSId(newName, sbmlModel);
  std::string sId{id.toStdString()};
  SPDLOG_INFO("  - id: {}", sId);
  reac->setId(sId);
  ids.push_back(id);
  parameterIds.push_back({});
  reac->setFast(false);
  reac->setCompartment(locationId.toStdString());
  reac->setReversible(true);
  auto *srp =
      static_cast<libsbml::SpatialReactionPlugin *>(reac->getPlugin("spatial"));
  srp->setIsLocal(true);
  SPDLOG_INFO("  - location: {}", reac->getCompartment());
  auto *kin = reac->createKineticLaw();
  kin->setFormula(rateExpression.toStdString());
  return newName;
}

void ModelReactions::remove(const QString &id) {
  auto i = ids.indexOf(id);
  std::string sId = id.toStdString();
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
      reactionIdsToRemove.push_back(reaction->getId().c_str());
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
  auto *reac = sbmlModel->getReaction(sId);
  SPDLOG_INFO("sId '{}' : name -> '{}'", sId, sName);
  reac->setName(sName);
  return uniqueName;
}

QString ModelReactions::getName(const QString &id) const {
  auto i = ids.indexOf(id);
  if (i < 0) {
    return {};
  }
  return names[i];
}

QString ModelReactions::getScheme(const QString &id) const {
  const auto *reac{sbmlModel->getReaction(id.toStdString())};
  if (reac == nullptr) {
    SPDLOG_WARN("Reaction '{}' not found", id.toStdString());
    return {};
  }
  QString lhs;
  const unsigned numReactants{reac->getNumReactants()};
  for (unsigned i = 0; i < numReactants; ++i) {
    const auto *r{reac->getReactant(i)};
    const auto &sName{sbmlModel->getSpecies(r->getSpecies())->getName()};
    if (double stoich{r->getStoichiometry()}; stoich != 0) {
      if (stoich == 1.0) {
      } else if (std::floor(stoich) == stoich) {
        lhs.append(QString::number(static_cast<int>(stoich)));
        lhs.append(" ");
      } else {
        lhs.append(QString::number(stoich));
        lhs.append(" ");
      }
      lhs.append(sName.c_str());
      lhs.append(" + ");
    }
  }
  lhs.chop(3);
  QString rhs;
  const unsigned numProducts{reac->getNumProducts()};
  for (unsigned i = 0; i < numProducts; ++i) {
    const auto *r{reac->getProduct(i)};
    const auto &sName{sbmlModel->getSpecies(r->getSpecies())->getName()};
    if (double stoich{r->getStoichiometry()}; stoich != 0) {
      if (stoich == 1.0) {
      } else if (std::floor(stoich) == stoich) {
        rhs.append(QString::number(static_cast<int>(stoich)));
        rhs.append(" ");
      } else {
        rhs.append(QString::number(stoich));
        rhs.append(" ");
      }
      rhs.append(sName.c_str());
      rhs.append(" + ");
    }
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
  SPDLOG_INFO("Setting reaction '{}' location to '{}'", id.toStdString(),
              locationId.toStdString());
  reaction->setCompartment(locationId.toStdString());
  removeInvalidSpecies(reaction, modelMembranes->getMembranes());
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
  if (const auto *product = reaction->getProduct(sId); product != nullptr) {
    stoichiometry += product->getStoichiometry();
  }
  if (const auto *reactant = reaction->getReactant(sId); reactant != nullptr) {
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
  auto *kin{getOrCreateKineticLaw(sbmlModel, id)};
  SPDLOG_INFO("  - expr: {}", expression.toStdString());
  std::unique_ptr<libsbml::ASTNode> exprAST(
      libsbml::SBML_parseL3Formula(expression.toStdString().c_str()));
  if (exprAST == nullptr) {
    SPDLOG_ERROR("SBML failed to parse expression: {}",
                 libsbml::SBML_getLastParseL3Error());
    return;
  }
  hasUnsavedChanges = true;
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
    parNames.push_back(getParameterName(reactionId, id));
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
  auto *reac = sbmlModel->getReaction(reactionId.toStdString());
  auto *kin = reac->getKineticLaw();
  SPDLOG_INFO("  - name: {}", uniqueName.toStdString());
  auto *param = kin->createLocalParameter();
  param->setName(uniqueName.toStdString());
  auto id = nameToUniqueSId(uniqueName, sbmlModel);
  std::string sId{id.toStdString()};
  SPDLOG_INFO("  - id: {}", sId);
  param->setId(sId);
  pars.push_back(id);
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
  auto *reac{sbmlModel->getReaction(reactionId.toStdString())};
  if (reac == nullptr || !reac->isSetKineticLaw()) {
    SPDLOG_WARN("reaction '{}' not found in sbml or has no kinetic law",
                reactionId.toStdString());
    return;
  }
  if (std::unique_ptr<libsbml::LocalParameter> rmParam{
          reac->getKineticLaw()->removeLocalParameter(id.toStdString())};
      rmParam != nullptr) {
    SPDLOG_INFO("  - removed LocalParameter '{}' from Reaction '{}'",
                rmParam->getId(), reac->getId());
    hasUnsavedChanges = true;
  }
}

bool ModelReactions::dependOnVariable(const QString &variableId) const {
  auto v{variableId.toStdString()};
  return std::any_of(ids.begin(), ids.end(), [&v, this](const auto &id) {
    auto e{getRateExpression(id).toStdString()};
    return utils::symbolicContains(e, v);
  });
}

bool ModelReactions::getHasUnsavedChanges() const { return hasUnsavedChanges; }

void ModelReactions::setHasUnsavedChanges(bool unsavedChanges) {
  hasUnsavedChanges = unsavedChanges;
}

} // namespace sme::model
