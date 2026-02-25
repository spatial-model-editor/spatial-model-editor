// SBML parameters

#pragma once

#include <QColor>
#include <QStringList>
#include <map>
#include <optional>
#include <string>

namespace libsbml {
class Model;
}

namespace sme::model {

class ModelParameters;
class ModelSpecies;

class ModelEvents {
private:
  QStringList ids;
  QStringList names;
  libsbml::Model *sbmlModel{};
  ModelParameters *modelParameters{};
  ModelSpecies *modelSpecies{};
  bool hasUnsavedChanges{false};

public:
  /**
   * @brief Construct empty event model.
   */
  ModelEvents();
  /**
   * @brief Construct event model from SBML model.
   */
  explicit ModelEvents(libsbml::Model *model,
                       ModelParameters *parameters = nullptr,
                       ModelSpecies *species = nullptr);
  /**
   * @brief Event ids.
   */
  [[nodiscard]] const QStringList &getIds() const;
  /**
   * @brief Event names.
   */
  [[nodiscard]] const QStringList &getNames() const;
  /**
   * @brief Set display name.
   */
  QString setName(const QString &id, const QString &name);
  /**
   * @brief Get display name.
   */
  [[nodiscard]] QString getName(const QString &id) const;
  /**
   * @brief Set target variable id modified by event.
   */
  void setVariable(const QString &id, const QString &variable);
  /**
   * @brief Get target variable id.
   */
  [[nodiscard]] QString getVariable(const QString &id) const;
  /**
   * @brief Set trigger time for event.
   */
  void setTime(const QString &id, double time);
  /**
   * @brief Get trigger time for event.
   */
  [[nodiscard]] double getTime(const QString &id) const;
  /**
   * @brief Set assignment expression.
   */
  void setExpression(const QString &id, const QString &expr);
  /**
   * @brief Get assignment expression.
   */
  [[nodiscard]] QString getExpression(const QString &id) const;
  /**
   * @brief Add new event.
   */
  QString add(const QString &name, const QString &variable);
  /**
   * @brief Returns ``true`` if id refers to a parameter.
   */
  [[nodiscard]] bool isParameter(const QString &id) const;
  /**
   * @brief Get current value of event target variable.
   */
  [[nodiscard]] double getValue(const QString &id) const;
  /**
   * @brief Remove event.
   */
  void remove(const QString &id);
  /**
   * @brief Remove events targeting the supplied variable.
   */
  void removeAnyUsingVariable(const QString &variable);
  /**
   * @brief Apply event assignment immediately.
   */
  void applyEvent(const QString &id);
  /**
   * @brief Unsaved state flag.
   */
  [[nodiscard]] bool getHasUnsavedChanges() const;
  /**
   * @brief Set unsaved state flag.
   */
  void setHasUnsavedChanges(bool unsavedChanges);
};

} // namespace sme::model
