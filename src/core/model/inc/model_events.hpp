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
  ModelEvents();
  explicit ModelEvents(libsbml::Model *model,
                       ModelParameters *parameters = nullptr,
                       ModelSpecies *species = nullptr);
  [[nodiscard]] const QStringList &getIds() const;
  [[nodiscard]] const QStringList &getNames() const;
  QString setName(const QString &id, const QString &name);
  [[nodiscard]] QString getName(const QString &id) const;
  void setVariable(const QString &id, const QString &variable);
  [[nodiscard]] QString getVariable(const QString &id) const;
  void setTime(const QString &id, double time);
  [[nodiscard]] double getTime(const QString &id) const;
  void setExpression(const QString &id, const QString &expr);
  [[nodiscard]] QString getExpression(const QString &id) const;
  QString add(const QString &name, const QString &variable);
  [[nodiscard]] bool isParameter(const QString &id) const;
  [[nodiscard]] double getValue(const QString &id) const;
  void remove(const QString &id);
  void removeAnyUsingVariable(const QString &variable);
  void applyEvent(const QString &id);
  [[nodiscard]] bool getHasUnsavedChanges() const;
  void setHasUnsavedChanges(bool unsavedChanges);
};

} // namespace sme::model
