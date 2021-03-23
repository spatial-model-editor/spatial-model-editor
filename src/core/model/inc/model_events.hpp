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
  const QStringList &getIds() const;
  const QStringList &getNames() const;
  QString setName(const QString &id, const QString &name);
  QString getName(const QString &id) const;
  void setVariable(const QString &id, const QString &variable);
  QString getVariable(const QString &id) const;
  void setTime(const QString &id, double time);
  double getTime(const QString &id) const;
  void setExpression(const QString &id, const QString &expr);
  QString getExpression(const QString &id) const;
  QString add(const QString &name, const QString &variable);
  void remove(const QString &id);
  void removeAnyUsingVariable(const QString &variable);
  void applyEvent(const QString &id);
  bool getHasUnsavedChanges() const;
  void setHasUnsavedChanges(bool unsavedChanges);
};

} // namespace sme::model
