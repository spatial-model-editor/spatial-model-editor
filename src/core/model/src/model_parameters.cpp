#include "model_parameters.hpp"

#include <sbml/SBMLTypes.h>
#include <sbml/extension/SBMLDocumentPlugin.h>
#include <sbml/packages/spatial/common/SpatialExtensionTypes.h>
#include <sbml/packages/spatial/extension/SpatialExtension.h>

#include <QString>
#include <memory>

#include "id.hpp"
#include "logger.hpp"
#include "math.hpp"
#include "model_species.hpp"
#include "sbml_utils.hpp"
#include "xml_annotation.hpp"

namespace model {

static bool isUserVisibleParameter(const libsbml::Parameter *param) {
  if (const auto *spp = static_cast<const libsbml::SpatialParameterPlugin *>(
          param->getPlugin("spatial"));
      spp != nullptr && spp->isSpatialParameter()) {
    // spatial parameters are set elsewhere in the GUI
    return false;
  }
  return true;
}

static QStringList importIds(const libsbml::Model *model) {
  QStringList ids;
  unsigned int numParams = model->getNumParameters();
  ids.reserve(static_cast<int>(numParams));
  for (unsigned int i = 0; i < numParams; ++i) {
    const auto *param = model->getParameter(i);
    if (isUserVisibleParameter(param)) {
      ids.push_back(param->getId().c_str());
    }
  }
  return ids;
}

static QStringList importNamesAndMakeUnique(const QStringList &ids,
                                            libsbml::Model *model) {
  QStringList names;
  unsigned int numParams = model->getNumParameters();
  names.reserve(static_cast<int>(numParams));
  for (const auto &id : ids) {
    auto *param = model->getParameter(id.toStdString());
    auto sId = param->getId();
    if (param->getName().empty()) {
      SPDLOG_INFO("Parameter '{0}' has no Name, using '{0}'", sId);
      param->setName(sId);
    }
    std::string name = param->getName();
    while (names.contains(name.c_str())) {
      name.append("_");
      param->setName(name);
      SPDLOG_INFO("Changing Parameter '{}' name to '{}' to make it unique", sId,
                  name);
    }
    names.push_back(QString::fromStdString(name));
  }
  return names;
}

ModelParameters::ModelParameters() = default;

ModelParameters::ModelParameters(libsbml::Model *model)
    : ids{importIds(model)}, names{importNamesAndMakeUnique(ids, model)},
      sbmlModel{model} {}

const QStringList &ModelParameters::getIds() const { return ids; }

const QStringList &ModelParameters::getNames() const { return names; }

QString ModelParameters::setName(const QString &id, const QString &name) {
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
  auto *param = sbmlModel->getParameter(sId);
  if (param == nullptr) {
    SPDLOG_ERROR("Parameter {} not found", sId);
    return {};
  }
  SPDLOG_INFO("sId '{}' : name -> '{}'", sId, sName);
  param->setName(sName);
  return uniqueName;
}

QString ModelParameters::getName(const QString &id) const {
  auto i = ids.indexOf(id);
  if (i < 0) {
    return {};
  }
  return names[i];
}

void ModelParameters::setExpression(const QString &id, const QString &expr) {
  std::string sId{id.toStdString()};
  auto *param = sbmlModel->getParameter(sId);
  if (param == nullptr) {
    SPDLOG_ERROR("Parameter '{}' not found", sId);
    return;
  }
  bool validDouble{false};
  double value = expr.toDouble(&validDouble);
  if (validDouble) {
    SPDLOG_INFO("Setting Parameter '{}' to constant double {}", sId, value);
    param->setValue(value);
    param->setConstant(true);
    if (auto *asgn = sbmlModel->getAssignmentRuleByVariable(sId);
        asgn != nullptr) {
      SPDLOG_INFO("  -> removing assignment rule '{}'", asgn->getId());
      asgn->removeFromParentAndDelete();
    }
  } else {
    SPDLOG_INFO("Setting Parameter '{}' to non-constant", sId, value);
    param->unsetValue();
    param->setConstant(false);
    auto *asgn = sbmlModel->getAssignmentRuleByVariable(sId);
    if (asgn == nullptr) {
      asgn = sbmlModel->createAssignmentRule();
      asgn->setId(nameToUniqueSId(QString("%1_assignment").arg(id), sbmlModel)
                      .toStdString());
      asgn->setVariable(sId);
      SPDLOG_INFO("  -> creating assignment rule '{}' for variable '{}'",
                  asgn->getId(), sId);
    }
    std::unique_ptr<const libsbml::ASTNode> astNode{
        mathStringToAST(expr.toStdString(), sbmlModel)};
    if (astNode == nullptr) {
      std::unique_ptr<char, decltype(&std::free)> err(
          libsbml::SBML_getLastParseL3Error(), &std::free);
      SPDLOG_ERROR("{}", err.get());
      return;
    }
    asgn->setMath(astNode.get());
    SPDLOG_INFO("  -> assignment rule expression '{}'",
                mathASTtoString(astNode.get()));
  }
}

QString ModelParameters::getExpression(const QString &id) const {
  std::string sId{id.toStdString()};
  const auto *param = sbmlModel->getParameter(sId);
  if (param == nullptr) {
    SPDLOG_ERROR("Parameter '{}' not found", sId);
    return {};
  }
  if (const auto *asgn = sbmlModel->getAssignmentRuleByVariable(sId);
      asgn != nullptr) {
    return mathASTtoString(asgn->getMath()).c_str();
  }
  if (!param->isSetValue()) {
    SPDLOG_ERROR("Parameter '{}' value is not set", sId);
    return "0";
  }
  return QString::number(param->getValue(), 'g', 15);
}

QString ModelParameters::add(const QString &name) {
  auto paramId = nameToUniqueSId(name, sbmlModel).toStdString();
  QString uniqueName = name;
  while (names.contains(uniqueName)) {
    uniqueName.append("_");
  }
  std::string paramName{uniqueName.toStdString()};
  SPDLOG_INFO("Adding parameter");
  SPDLOG_INFO("  - Id: {}", paramId);
  SPDLOG_INFO("  - Name: {}", paramName);
  auto *param = sbmlModel->createParameter();
  param->setId(paramId);
  param->setName(paramName);
  param->setValue(0);
  ids.push_back(paramId.c_str());
  names.push_back(uniqueName);
  return uniqueName;
}

void ModelParameters::remove(const QString &id) {
  std::string sId{id.toStdString()};
  SPDLOG_INFO("Removing parameter {}", sId);
  if (auto *asgn = sbmlModel->getAssignmentRuleByVariable(sId);
      asgn != nullptr) {
    SPDLOG_INFO("  - removing assignment rule '{}'", asgn->getId());
    asgn->removeFromParentAndDelete();
  }
  std::unique_ptr<libsbml::Parameter> rmpar(sbmlModel->removeParameter(sId));
  if (rmpar == nullptr) {
    SPDLOG_WARN("  - parameter {} not found", sId);
    return;
  }
  SPDLOG_INFO("  - parameter {} removed", rmpar->getId());
  auto i = ids.indexOf(id);
  ids.removeAt(i);
  names.removeAt(i);
}

std::vector<IdName> ModelParameters::getSymbols() const {
  std::vector<IdName> symbols;
  for (int i = 0; i < ids.size(); ++i) {
    symbols.push_back({ids[i].toStdString(), names[i].toStdString()});
  }
  for (unsigned i = 0; i < sbmlModel->getNumSpecies(); ++i) {
    const auto *spec = sbmlModel->getSpecies(i);
    symbols.push_back({spec->getId(), spec->getName()});
  }
  return symbols;
}

std::vector<IdValue> ModelParameters::getGlobalConstants() const {
  std::vector<IdValue> constants;
  // add all *constant* species as constants
  for (unsigned k = 0; k < sbmlModel->getNumSpecies(); ++k) {
    const auto *spec = sbmlModel->getSpecies(k);
    if (getIsSpeciesConstant(spec)) {
      SPDLOG_TRACE("found constant species {}", spec->getId());
      double init_conc = spec->getInitialConcentration();
      constants.push_back({spec->getId(), init_conc});
      SPDLOG_TRACE("parameter {} = {}", spec->getId(), init_conc);
    }
  }
  // add any parameters (that are not replaced by an AssignmentRule)
  for (unsigned k = 0; k < sbmlModel->getNumParameters(); ++k) {
    const auto *param = sbmlModel->getParameter(k);
    if (sbmlModel->getAssignmentRule(param->getId()) == nullptr) {
      SPDLOG_TRACE("parameter {} = {}", param->getId(), param->getValue());
      if (!(param->getId() == "x" || param->getId() == "y")) {
        // remove x and y if present, as these are not really parameters
        // (we want them to remain as variables to be parsed by symbolic
        // parser) todo: check if this can be done in a better way
        constants.push_back({param->getId(), param->getValue()});
      }
    }
  }
  // also get compartment volumes (the compartmentID may be used in the
  // reaction equation, and it should be replaced with the value of the "Size"
  // parameter for this compartment)
  for (unsigned int k = 0; k < sbmlModel->getNumCompartments(); ++k) {
    const auto *comp = sbmlModel->getCompartment(k);
    SPDLOG_TRACE("parameter {} = {}", comp->getId(), comp->getSize());
    constants.push_back({comp->getId(), comp->getSize()});
  }
  return constants;
}

std::vector<IdNameExpr> ModelParameters::getNonConstantParameters() const {
  std::vector<IdNameExpr> rules;
  for (unsigned k = 0; k < sbmlModel->getNumRules(); ++k) {
    if (const auto *rule = sbmlModel->getRule(k); rule->isAssignment()) {
      const auto &sId = rule->getVariable();
      rules.push_back(
          {sId, sbmlModel->getParameter(sId)->getName(), rule->getFormula()});
    }
  }
  return rules;
}

} // namespace model
