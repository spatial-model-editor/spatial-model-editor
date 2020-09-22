#include "model_reactions.hpp"

#include <sbml/SBMLTypes.h>
#include <sbml/extension/SBMLDocumentPlugin.h>
#include <sbml/packages/spatial/common/SpatialExtensionTypes.h>
#include <sbml/packages/spatial/extension/SpatialExtension.h>

#include <QString>
#include <memory>

#include "geometry.hpp"
#include "id.hpp"
#include "logger.hpp"
#include "sbml_math.hpp"
#include "symbolic.hpp"
#include "utils.hpp"
#include "xml_annotation.hpp"

namespace model {

static QStringList importIds(const libsbml::Model *model) {
  QStringList ids;
  for (unsigned int i = 0; i < model->getNumReactions(); ++i) {
    const auto *reac = model->getReaction(i);
    ids.push_back(reac->getId().c_str());
  }
  return ids;
}

static QStringList importNamesAndMakeUnique(libsbml::Model *model) {
  QStringList names;
  // get all species, make a list for each compartment
  for (unsigned int i = 0; i < model->getNumReactions(); ++i) {
    auto *reac = model->getReaction(i);
    const auto &sId = reac->getId();
    if (reac->getName().empty()) {
      SPDLOG_INFO("Reaction '{}' has no Name, using sId", sId);
      reac->setName(sId);
    }
    auto name = makeUnique(reac->getName().c_str(), names);
    reac->setName(name.toStdString());
    names.push_back(name);
  }
  return names;
}

static QVector<QStringList> importParameterIds(const libsbml::Model *model) {
  QVector<QStringList> paramIds;
  for (unsigned int i = 0; i < model->getNumReactions(); ++i) {
    const auto *reac = model->getReaction(i);
    const auto *kin = reac->getKineticLaw();
    paramIds.push_back({});
    auto &ids = paramIds.back();
    for (unsigned int j = 0; j < kin->getNumLocalParameters(); ++j) {
      ids.push_back(kin->getLocalParameter(j)->getId().c_str());
    }
  }
  return paramIds;
}

static bool
makeReactionSpatial(libsbml::Reaction *reac,
                    const std::vector<geometry::Membrane> &membranes) {
  const auto *model = reac->getModel();
  utils::SmallStackSet<std::string, 3> compSet;
  if (reac->isSetCompartment()) {
    compSet.insert(reac->getCompartment());
  }
  for (unsigned int k = 0; k < reac->getNumProducts(); ++k) {
    const auto &sId = reac->getProduct(k)->getSpecies();
    const auto &compId = model->getSpecies(sId)->getCompartment();
    compSet.insert(compId);
  }
  for (unsigned int k = 0; k < reac->getNumReactants(); ++k) {
    const auto &sId = reac->getReactant(k)->getSpecies();
    const auto &compId = model->getSpecies(sId)->getCompartment();
    compSet.insert(compId);
  }
  for (unsigned int k = 0; k < reac->getNumModifiers(); ++k) {
    const auto &sId = reac->getModifier(k)->getSpecies();
    const auto &compId = model->getSpecies(sId)->getCompartment();
    compSet.insert(compId);
  }
  const auto *kin = reac->getKineticLaw();
  if (kin == nullptr) {
    kin = reac->createKineticLaw();
  }
  if (compSet.size() == 1) {
    if (!reac->isSetCompartment()) {
      SPDLOG_INFO("Reaction compartment not set: using species location '{}'",
                  compSet[0]);
      reac->setCompartment(compSet[0]);
    }
    SPDLOG_INFO("Reaction involves species from a single compartment");
    SPDLOG_INFO("  - original rate units: d[amount]/dt");
    SPDLOG_INFO("  -> want spatial compartment reaction: d[concentration]/dt");
    SPDLOG_INFO("  -> dividing rate by compartment size");
    auto expr = mathASTtoString(kin->getMath());
    SPDLOG_INFO("  - {}", expr);
    auto newExpr = symbolic::divide(expr, compSet[0]);
    SPDLOG_INFO("  --> {}", newExpr);
    std::unique_ptr<libsbml::ASTNode> argAST(
        libsbml::SBML_parseL3Formula(newExpr.c_str()));
    if (argAST != nullptr) {
      reac->getKineticLaw()->setMath(argAST.get());
      SPDLOG_INFO("  - new math: {}",
                  mathASTtoString(reac->getKineticLaw()->getMath()));
    } else {
      SPDLOG_ERROR("  - libSBML failed to parse expression");
    }
    return true;
  } else if (compSet.size() == 2) {
    SPDLOG_INFO("Reaction involves species from two compartments:");
    SPDLOG_INFO("  - '{}'", compSet[0]);
    SPDLOG_INFO("  - '{}'", compSet[1]);
    SPDLOG_INFO("  - original rate units: d[amount]/dt");
    SPDLOG_INFO(
        "  -> want spatial membrane reaction: d[amount]/d[membrane area]/dt");
    SPDLOG_WARN("  -> but NOT changing rate automatically");
    for (const auto &membrane : membranes) {
      const auto &cA = membrane.getCompartmentA()->getId();
      const auto &cB = membrane.getCompartmentB()->getId();
      if ((cA == compSet[0] && cB == compSet[1]) ||
          (cA == compSet[1] && cB == compSet[0])) {
        SPDLOG_INFO("  -> setting reaction location to Membrane '{}'",
                    membrane.getId());
        reac->setCompartment(membrane.getId());
        return true;
      }
    }
    return false;
  } else {
    SPDLOG_WARN(
        "Reaction involves species from {} compartments - not supported",
        compSet.size());
  }
  return false;
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

ModelReactions::ModelReactions() = default;

ModelReactions::ModelReactions(libsbml::Model *model,
                               const std::vector<geometry::Membrane> &membranes)
    : ids{importIds(model)}, names{importNamesAndMakeUnique(model)},
      parameterIds{importParameterIds(model)}, sbmlModel{model} {
  makeReactionsSpatial(membranes);
}

void ModelReactions::makeReactionsSpatial(
    const std::vector<geometry::Membrane> &membranes) {
  if (sbmlModel == nullptr) {
    return;
  }
  for (unsigned int i = 0; i < sbmlModel->getNumReactions(); ++i) {
    auto *reac = sbmlModel->getReaction(i);
    reac->setFast(false);
    if (const auto *kin = reac->getKineticLaw(); kin == nullptr) {
      kin = reac->createKineticLaw();
    }
    auto *srp = static_cast<libsbml::SpatialReactionPlugin *>(
        reac->getPlugin("spatial"));
    if (srp != nullptr && !(srp->isSetIsLocal() && srp->getIsLocal())) {
      if (makeReactionSpatial(reac, membranes)) {
        SPDLOG_INFO("Setting isLocal=true for reaction {}", reac->getId());
        srp->setIsLocal(true);
      }
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
  ids.removeAt(i);
  names.removeAt(i);
}

void ModelReactions::removeAllInvolvingSpecies(const QString &speciesId) {
  QStringList reacIdsToRemove;
  for (unsigned i = 0; i < sbmlModel->getNumReactions(); ++i) {
    const auto *reac = sbmlModel->getReaction(i);
    if (reactionInvolvesSpecies(reac, speciesId.toStdString())) {
      SPDLOG_INFO("  - removing reaction {}", reac->getId());
      reacIdsToRemove.push_back(reac->getId().c_str());
    }
  }
  for (const auto &reacId : reacIdsToRemove) {
    remove(reacId);
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

void ModelReactions::setLocation(const QString &id, const QString &locationId) {
  auto *reac = sbmlModel->getReaction(id.toStdString());
  if (reac == nullptr) {
    SPDLOG_WARN("Reaction '{}' not found", id.toStdString());
    return;
  }
  SPDLOG_INFO("Setting reaction '{}' location to '{}'", id.toStdString(),
              locationId.toStdString());
  reac->setCompartment(locationId.toStdString());
}

QString ModelReactions::getLocation(const QString &id) const {
  const auto *reac = sbmlModel->getReaction(id.toStdString());
  if (reac == nullptr) {
    return {};
  }
  return reac->getCompartment().c_str();
}

int ModelReactions::getSpeciesStoichiometry(const QString &id,
                                            const QString &speciesId) const {
  const auto *reac = sbmlModel->getReaction(id.toStdString());
  std::string sId = speciesId.toStdString();
  int stoichiometry = 0;
  if (const auto *product = reac->getProduct(sId); product != nullptr) {
    stoichiometry += static_cast<int>(product->getStoichiometry());
  }
  if (const auto *reactant = reac->getReactant(sId); reactant != nullptr) {
    stoichiometry -= static_cast<int>(reactant->getStoichiometry());
  }
  return stoichiometry;
}

void ModelReactions::setSpeciesStoichiometry(const QString &id,
                                             const QString &speciesId,
                                             int stoichiometry) {
  auto *reac = sbmlModel->getReaction(id.toStdString());
  std::string sId = speciesId.toStdString();
  const auto *spec = sbmlModel->getSpecies(sId);
  SPDLOG_TRACE("Reaction '{}', Species '{}', Stoichiometry {}", reac->getId(),
               spec->getId(), stoichiometry);
  if (std::unique_ptr<libsbml::SpeciesReference> rmProduct(
          reac->removeProduct(sId));
      rmProduct != nullptr) {
    SPDLOG_TRACE("  - removed Product '{}'", rmProduct->getSpecies());
  }
  if (std::unique_ptr<libsbml::SpeciesReference> rmReactant(
          reac->removeReactant(sId));
      rmReactant != nullptr) {
    SPDLOG_TRACE("  - removed Reactant '{}'", rmReactant->getSpecies());
  }
  if (stoichiometry > 0) {
    reac->addProduct(spec, static_cast<double>(stoichiometry));
    SPDLOG_TRACE("  - adding Product '{}' with stoichiometry {}", spec->getId(),
                 stoichiometry);
  }
  if (stoichiometry < 0) {
    reac->addReactant(spec, static_cast<double>(-stoichiometry));
    SPDLOG_TRACE("  - adding Reactant '{}' with stoichiometry {}",
                 spec->getId(), -stoichiometry);
  }
}

QString ModelReactions::getRateExpression(const QString &id) const {
  const auto *reac = sbmlModel->getReaction(id.toStdString());
  if (reac == nullptr) {
    return {};
  }
  const auto *kin = reac->getKineticLaw();
  if (kin == nullptr) {
    return {};
  }
  return kin->getFormula().c_str();
}

static libsbml::KineticLaw *getOrCreateKineticLaw(libsbml::Model *model,
                                                  const QString &reactionId) {
  auto *reac = model->getReaction(reactionId.toStdString());
  auto *kin = reac->getKineticLaw();
  if (kin == nullptr) {
    kin = reac->createKineticLaw();
  }
  return kin;
}

void ModelReactions::setRateExpression(const QString &id,
                                       const QString &expression) {
  auto *kin = getOrCreateKineticLaw(sbmlModel, id);
  SPDLOG_INFO("  - expr: {}", expression.toStdString());
  std::unique_ptr<libsbml::ASTNode> exprAST(
      libsbml::SBML_parseL3Formula(expression.toStdString().c_str()));
  if (exprAST == nullptr) {
    SPDLOG_ERROR("SBML failed to parse expression: {}",
                 libsbml::SBML_getLastParseL3Error());
    return;
  }
  kin->setMath(exprAST.get());
}

static libsbml::LocalParameter *getLocalParameter(libsbml::Model *model,
                                                  const QString &reactionId,
                                                  const QString &parameterId) {
  auto *reac = model->getReaction(reactionId.toStdString());
  auto *kin = reac->getKineticLaw();
  return kin->getLocalParameter(parameterId.toStdString());
}

QStringList ModelReactions::getParameterIds(const QString &id) const {
  auto i = ids.indexOf(id);
  return parameterIds[i];
}

QString ModelReactions::setParameterName(const QString &reactionId,
                                         const QString &parameterId,
                                         const QString &name) {
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
  auto *param = getLocalParameter(sbmlModel, reactionId, parameterId);
  param->setValue(value);
}

double ModelReactions::getParameterValue(const QString &reactionId,
                                         const QString &parameterId) const {
  const auto *param = getLocalParameter(sbmlModel, reactionId, parameterId);
  return param->getValue();
}

QString ModelReactions::addParameter(const QString &reactionId,
                                     const QString &name, double value) {
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
  auto iReac = ids.indexOf(reactionId);
  auto wasRemoved = parameterIds[iReac].removeOne(id);
  SPDLOG_DEBUG(" removed '{}' from parameterIds: {} for Reaction '{}'",
               id.toStdString(), reactionId.toStdString(), wasRemoved);
  auto *reac = sbmlModel->getReaction(reactionId.toStdString());
  auto *kin = reac->getKineticLaw();
  if (std::unique_ptr<libsbml::LocalParameter> rmParam{
          kin->removeLocalParameter(id.toStdString())};
      rmParam != nullptr) {
    SPDLOG_INFO("  - removed LocalParameter '{}' from Reaction '{}'",
                rmParam->getId(), reac->getId());
  }
}
} // namespace model
