// SBML functions

#pragma once

#include "symbolic.hpp"
#include <QColor>
#include <QStringList>
#include <map>
#include <optional>
#include <string>

namespace libsbml {
class Model;
}

namespace sme {

namespace model {

class ModelFunctions {
private:
  QStringList ids;
  QStringList names;
  libsbml::Model *sbmlModel = nullptr;
  bool hasUnsavedChanges{false};

public:
  ModelFunctions();
  explicit ModelFunctions(libsbml::Model *model);
  [[nodiscard]] const QStringList &getIds() const;
  [[nodiscard]] const QStringList &getNames() const;
  QString setName(const QString &id, const QString &name);
  [[nodiscard]] QString getName(const QString &id) const;
  void setExpression(const QString &id, const QString &expression);
  [[nodiscard]] QString getExpression(const QString &id) const;
  [[nodiscard]] QStringList getArguments(const QString &id) const;
  QString addArgument(const QString &functionId, const QString &argumentId);
  void removeArgument(const QString &functionId, const QString &argumentId);
  QString add(const QString &name);
  void remove(const QString &id);
  [[nodiscard]] std::vector<common::SymbolicFunction>
  getSymbolicFunctions() const;
  [[nodiscard]] bool getHasUnsavedChanges() const;
  void setHasUnsavedChanges(bool unsavedChanges);
};

} // namespace model

} // namespace sme
