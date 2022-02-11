#include "sme/model_events.hpp"
#include "id.hpp"
#include "sbml_math.hpp"
#include "sbml_utils.hpp"
#include "sme/logger.hpp"
#include "sme/model_parameters.hpp"
#include "sme/model_species.hpp"
#include <QString>
#include <memory>
#include <sbml/SBMLTypes.h>
#include <sbml/extension/SBMLDocumentPlugin.h>
#include <sbml/packages/spatial/common/SpatialExtensionTypes.h>
#include <sbml/packages/spatial/extension/SpatialExtension.h>

namespace sme::model {

static void moveLastAssignmentToNewEvent(libsbml::Model *model,
                                         libsbml::Event *event) {
  // copy & rename event
  std::unique_ptr<libsbml::Event> eventClone(event->clone());
  // get a new event id
  SPDLOG_INFO("event id '{}'", event->getId());
  std::string newId{event->getId()};
  while (!isSIdAvailable(newId, model)) {
    newId.append("_");
  }
  eventClone->setId(newId);
  model->addEvent(eventClone.get());
  auto *newEvent{model->getEvent(newId)};
  SPDLOG_INFO("new event id '{}'", newEvent->getId());
  // remove last assignment from event
  event->getEventAssignment(event->getNumEventAssignments() - 1)
      ->removeFromParentAndDelete();
  // remove all except last assignment from new event
  while (newEvent->getNumEventAssignments() > 1) {
    newEvent->getEventAssignment(0)->removeFromParentAndDelete();
  }
}

static void splitMultipleEventAssignments(libsbml::Model *model) {
  std::vector<libsbml::Event *> eventsToSplit;
  unsigned int numEvents{model->getNumEvents()};
  for (unsigned int i = 0; i < numEvents; ++i) {
    if (auto *event{model->getEvent(i)}; event->getNumEventAssignments() > 1) {
      eventsToSplit.push_back(event);
    }
  }
  for (auto *event : eventsToSplit) {
    while (event->getNumEventAssignments() > 1) {
      moveLastAssignmentToNewEvent(model, event);
    }
  }
}

static QStringList importIds(libsbml::Model *model) {
  QStringList ids;
  splitMultipleEventAssignments(model);
  unsigned int numEvents{model->getNumEvents()};
  ids.reserve(static_cast<int>(numEvents));
  for (unsigned int i = 0; i < numEvents; ++i) {
    const auto *event{model->getEvent(i)};
    ids.push_back(event->getId().c_str());
  }
  return ids;
}

static QStringList importNamesAndMakeUnique(const QStringList &ids,
                                            libsbml::Model *model) {
  QStringList names;
  unsigned int numEvents{model->getNumEvents()};
  names.reserve(static_cast<int>(numEvents));
  for (const auto &id : ids) {
    auto *event{model->getEvent(id.toStdString())};
    const auto &sId{event->getId()};
    if (event->getName().empty()) {
      SPDLOG_INFO("Event '{0}' has no Name, using '{0}'", sId);
      event->setName(sId);
    }
    std::string name = event->getName();
    while (names.contains(name.c_str())) {
      name.append("_");
      event->setName(name);
      SPDLOG_INFO("Changing Event '{}' name to '{}' to make it unique", sId,
                  name);
    }
    names.push_back(QString::fromStdString(name));
  }
  return names;
}

ModelEvents::ModelEvents() = default;

ModelEvents::ModelEvents(libsbml::Model *model, ModelParameters *parameters,
                         ModelSpecies *species)
    : ids{importIds(model)}, names{importNamesAndMakeUnique(ids, model)},
      sbmlModel{model}, modelParameters{parameters}, modelSpecies{species} {
  // try to import time from triggers: if this fails just set a default trigger
  for (const auto &id : ids) {
    double t{getTime(id)};
    setTime(id, t);
  }
  // todo: also validate variables & expressions
  // we want all events to be valid (for sme & sbml) by the end of this
  // constructor
}

const QStringList &ModelEvents::getIds() const { return ids; }

const QStringList &ModelEvents::getNames() const { return names; }

QString ModelEvents::setName(const QString &id, const QString &name) {
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
  auto *event = sbmlModel->getEvent(sId);
  if (event == nullptr) {
    SPDLOG_ERROR("Event {} not found", sId);
    return {};
  }
  SPDLOG_INFO("sId '{}' : name -> '{}'", sId, sName);
  event->setName(sName);
  return uniqueName;
}

QString ModelEvents::getName(const QString &id) const {
  auto i = ids.indexOf(id);
  if (i < 0) {
    return {};
  }
  return names[i];
}

void ModelEvents::setVariable(const QString &id, const QString &variable) {
  auto sId{id.toStdString()};
  auto vId{variable.toStdString()};
  SPDLOG_INFO("Setting event '{}' variable to '{}'", sId, vId);
  auto *event{sbmlModel->getEvent(sId)};
  if (event == nullptr || event->getNumEventAssignments() == 0) {
    SPDLOG_WARN("Event not found, or contains no EventAssignments");
    return;
  }
  if (auto *param{sbmlModel->getParameter(vId)}; param != nullptr) {
    hasUnsavedChanges = true;
    // parameter cannot be constant if modified by an event
    param->setConstant(false);
    event->getEventAssignment(0)->setVariable(vId);
    return;
  } else if (const auto *species{sbmlModel->getSpecies(vId)};
             species != nullptr) {
    hasUnsavedChanges = true;
    event->getEventAssignment(0)->setVariable(vId);
    return;
  }
  SPDLOG_WARN("Variable '{}' not found in sbml model", vId);
}

QString ModelEvents::getVariable(const QString &id) const {
  const auto *event{sbmlModel->getEvent(id.toStdString())};
  if (event == nullptr || event->getNumEventAssignments() == 0) {
    SPDLOG_WARN("Event not found, or contains no EventAssignments");
    return {};
  }
  const auto *asgn{event->getEventAssignment(0)};
  if (!asgn->isSetVariable()) {
    SPDLOG_WARN("Variable not set in EventAssignment");
    return {};
  }
  return asgn->getVariable().c_str();
}

void ModelEvents::setTime(const QString &id, double time) {
  auto triggerExpression{fmt::format("time >= {}", time)};
  SPDLOG_INFO("Setting event '{}' trigger to '{}'", id.toStdString(),
              triggerExpression);
  auto *event{sbmlModel->getEvent(id.toStdString())};
  if (event == nullptr || !event->isSetTrigger()) {
    SPDLOG_WARN("Event not found, or contains no Trigger");
    return;
  }
  hasUnsavedChanges = true;
  auto ast{mathStringToAST(triggerExpression, sbmlModel)};
  auto *trigger{event->getTrigger()};
  trigger->setMath(ast.get());
}

double ModelEvents::getTime(const QString &id) const {
  double defaultTime{0};
  const auto *event{sbmlModel->getEvent(id.toStdString())};
  if (event == nullptr || !event->isSetTrigger()) {
    SPDLOG_WARN("Event not found, or contains no Trigger: returning default {}",
                defaultTime);
    return defaultTime;
  }
  const auto *ast{event->getTrigger()->getMath()};
  if (ast == nullptr) {
    SPDLOG_WARN("Trigger contains no math");
    return defaultTime;
  }
  if (const auto *rhs{ast->getRightChild()};
      rhs != nullptr && rhs->isNumber()) {
    return rhs->getValue();
  }
  if (const auto *lhs{ast->getLeftChild()}; lhs != nullptr && lhs->isNumber()) {
    return lhs->getValue();
  }
  SPDLOG_WARN(
      "Trigger '{}' is not of supported form 'time >= 1' or '1 <= time'",
      mathASTtoString(ast));
  return defaultTime;
}

void ModelEvents::setExpression(const QString &id, const QString &expr) {
  std::string sId{id.toStdString()};
  auto *event = sbmlModel->getEvent(sId);
  if (event == nullptr || event->getNumEventAssignments() == 0) {
    SPDLOG_ERROR("Event '{}' not found or contains no event assignments", sId);
    return;
  }
  auto astNode{mathStringToAST(expr.toStdString(), sbmlModel)};
  if (astNode == nullptr) {
    std::unique_ptr<char, decltype(&std::free)> err(
        libsbml::SBML_getLastParseL3Error(), &std::free);
    SPDLOG_ERROR("{}", err.get());
    return;
  }
  hasUnsavedChanges = true;
  auto *asgn{event->getEventAssignment(0)};
  asgn->setMath(astNode.get());
  SPDLOG_INFO("Event '{}' expression -> '{}'", sId,
              mathASTtoString(astNode.get()));
}

QString ModelEvents::getExpression(const QString &id) const {
  std::string sId{id.toStdString()};
  const auto *event{sbmlModel->getEvent(sId)};
  if (event == nullptr || event->getNumEventAssignments() == 0) {
    SPDLOG_ERROR("Event '{}' not found or contains no event assignments", sId);
    return {};
  }
  return mathASTtoString(event->getEventAssignment(0)->getMath()).c_str();
}

QString ModelEvents::add(const QString &name, const QString &variable) {
  constexpr double defaultTime{0};
  std::string defaultValue{"0"};
  std::string eventVariable{variable.toStdString()};
  hasUnsavedChanges = true;
  auto eventId{nameToUniqueSId(name, sbmlModel).toStdString()};
  QString uniqueName{name};
  while (names.contains(uniqueName)) {
    uniqueName.append("_");
  }
  std::string eventName{uniqueName.toStdString()};
  SPDLOG_INFO("Adding event");
  auto *event{sbmlModel->createEvent()};
  SPDLOG_INFO("  - Id: {}", eventId);
  event->setId(eventId);
  SPDLOG_INFO("  - Name: {}", eventName);
  event->setName(eventName);
  SPDLOG_INFO("  - Trigger: {}", defaultTime);
  event->setUseValuesFromTriggerTime(false);
  auto *trigger{event->createTrigger()};
  trigger->setInitialValue(false);
  trigger->setPersistent(false);
  setTime(eventId.c_str(), defaultTime);
  SPDLOG_INFO("  - Variable: {}", eventVariable);
  SPDLOG_INFO("  - Value: {}", defaultValue);
  event->createEventAssignment();
  setVariable(eventId.c_str(), variable);
  setExpression(eventId.c_str(), defaultValue.c_str());
  ids.push_back(eventId.c_str());
  names.push_back(uniqueName);
  return uniqueName;
}

bool ModelEvents::isParameter(const QString &id) const {
  return sbmlModel->getParameter(getVariable(id).toStdString()) != nullptr;
}

double ModelEvents::getValue(const QString &id) const {
  bool validDouble{false};
  auto expr{getExpression(id)};
  double val{expr.toDouble(&validDouble)};
  if (!validDouble) {
    SPDLOG_WARN("Expression '{}' of event '{}' not a valid double",
                expr.toStdString(), id.toStdString());
  }
  return val;
}

void ModelEvents::remove(const QString &id) {
  std::string sId{id.toStdString()};
  SPDLOG_INFO("Removing event {}", sId);
  std::unique_ptr<libsbml::Event> rmev(sbmlModel->removeEvent(sId));
  if (rmev == nullptr) {
    SPDLOG_WARN("  - event {} not found", sId);
    return;
  }
  hasUnsavedChanges = true;
  SPDLOG_INFO("  - event {} removed", rmev->getId());
  auto i = ids.indexOf(id);
  ids.removeAt(i);
  names.removeAt(i);
}

void ModelEvents::removeAnyUsingVariable(const QString &variable) {
  QStringList idsToRemove;
  for (const auto &id : ids) {
    if (getVariable(id) == variable) {
      idsToRemove.push_back(id);
    }
  }
  for (const auto &id : idsToRemove) {
    remove(id);
  }
}

void ModelEvents::applyEvent(const QString &id) {
  std::string variableId{getVariable(id).toStdString()};
  SPDLOG_INFO("Applying event '{}' to model", id.toStdString());
  if (variableId.empty()) {
    SPDLOG_WARN("Variable not found for event '{}'", id.toStdString());
    return;
  }
  const auto &expr{getExpression(id)};
  if (const auto *param{sbmlModel->getParameter(variableId)};
      param != nullptr && modelParameters != nullptr) {
    modelParameters->setExpression(variableId.c_str(), expr);
    return;
  }
  if (const auto *species{sbmlModel->getSpecies(variableId)};
      species != nullptr && modelSpecies != nullptr) {
    modelSpecies->setAnalyticConcentration(variableId.c_str(), expr);
    return;
  }
  SPDLOG_WARN("Variable '{}' not found in sbml model", variableId);
}

bool ModelEvents::getHasUnsavedChanges() const { return hasUnsavedChanges; }

void ModelEvents::setHasUnsavedChanges(bool unsavedChanges) {
  hasUnsavedChanges = unsavedChanges;
}

} // namespace sme::model
