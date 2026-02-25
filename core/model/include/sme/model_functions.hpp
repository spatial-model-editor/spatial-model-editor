// SBML functions

#pragma once

#include "sme/symbolic.hpp"
#include <QColor>
#include <QStringList>
#include <map>
#include <optional>
#include <string>

namespace libsbml {
class Model;
}

namespace sme::model {

/**
 * @brief SBML function-definition manager.
 */
class ModelFunctions {
private:
  QStringList ids;
  QStringList names;
  libsbml::Model *sbmlModel = nullptr;
  bool hasUnsavedChanges{false};

public:
  /**
   * @brief Construct empty function model.
   */
  ModelFunctions();
  /**
   * @brief Construct function model from SBML model.
   */
  explicit ModelFunctions(libsbml::Model *model);
  /**
   * @brief Function ids.
   */
  [[nodiscard]] const QStringList &getIds() const;
  /**
   * @brief Function names.
   */
  [[nodiscard]] const QStringList &getNames() const;
  /**
   * @brief Set function display name.
   */
  QString setName(const QString &id, const QString &name);
  /**
   * @brief Get function display name.
   */
  [[nodiscard]] QString getName(const QString &id) const;
  /**
   * @brief Set function expression/body.
   */
  void setExpression(const QString &id, const QString &expression);
  /**
   * @brief Get function expression/body.
   */
  [[nodiscard]] QString getExpression(const QString &id) const;
  /**
   * @brief Get function argument ids.
   */
  [[nodiscard]] QStringList getArguments(const QString &id) const;
  /**
   * @brief Add argument to function.
   */
  QString addArgument(const QString &functionId, const QString &argumentId);
  /**
   * @brief Remove argument from function.
   */
  void removeArgument(const QString &functionId, const QString &argumentId);
  /**
   * @brief Add new function with default expression.
   */
  QString add(const QString &name);
  /**
   * @brief Remove function.
   */
  void remove(const QString &id);
  /**
   * @brief Convert to symbolic-function objects for expression processing.
   */
  [[nodiscard]] std::vector<common::SymbolicFunction>
  getSymbolicFunctions() const;
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
