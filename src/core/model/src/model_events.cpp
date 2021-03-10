#include "model_events.hpp"
#include "id.hpp"
#include "logger.hpp"
#include "sbml_math.hpp"
#include "sbml_utils.hpp"
#include <QString>
#include <memory>
#include <sbml/SBMLTypes.h>
#include <sbml/extension/SBMLDocumentPlugin.h>
#include <sbml/packages/spatial/common/SpatialExtensionTypes.h>
#include <sbml/packages/spatial/extension/SpatialExtension.h>

namespace sme::model {

static QStringList importIds(const libsbml::Model *model) {
  QStringList ids;
  unsigned int numEvents = model->getNumEvents();
  ids.reserve(static_cast<int>(numEvents));
  for (unsigned int i = 0; i < numEvents; ++i) {
    const auto *event = model->getEvent(i);
    ids.push_back(event->getId().c_str());
  }
  return ids;
}

static QStringList importNamesAndMakeUnique(const QStringList &ids,
                                            libsbml::Model *model) {
  QStringList names;
  unsigned int numEvents = model->getNumEvents();
  names.reserve(static_cast<int>(numEvents));
  for (const auto &id : ids) {
    auto *event = model->getEvent(id.toStdString());
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

ModelEvents::ModelEvents(libsbml::Model *model)
    : ids{importIds(model)}, names{importNamesAndMakeUnique(ids, model)},
      sbmlModel{model} {}

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
  auto *param{sbmlModel->getParameter(vId)};
  if (param == nullptr) {
    SPDLOG_WARN("Parameter '{}' not found", vId);
    return;
  }
  hasUnsavedChanges = true;
  // parameter cannot be constant if modified by an event
  param->setConstant(false);
  event->getEventAssignment(0)->setVariable(vId);
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
  auto triggerExpression{fmt::format("time == {}", time)};
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
      "Trigger '{}' is not of supported form 'time == 1' or '1 == time'",
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

bool ModelEvents::getHasUnsavedChanges() const { return hasUnsavedChanges; }

void ModelEvents::setHasUnsavedChanges(bool unsavedChanges) {
  hasUnsavedChanges = unsavedChanges;
}

} // namespace sme::model
