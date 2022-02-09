// SBML parameters

#pragma once

#include <QColor>
#include <QStringList>
#include <map>
#include <optional>
#include <string>

namespace libsbml {
class Model;
class Species;
} // namespace libsbml

namespace sme::model {

class ModelEvents;

struct IdName {
  std::string id;
  std::string name;
};

struct SpatialCoordinates {
  IdName x;
  IdName y;
  IdName z;
};

struct IdNameValue {
  std::string id;
  std::string name;
  double value;
};

struct IdNameExpr {
  std::string id;
  std::string name;
  std::string expr;
};

class ModelParameters {
private:
  QStringList ids;
  QStringList names;
  SpatialCoordinates spatialCoordinates;
  libsbml::Model *sbmlModel{nullptr};
  bool hasUnsavedChanges{false};
  ModelEvents *modelEvents{nullptr};

public:
  ModelParameters();
  explicit ModelParameters(libsbml::Model *model);
  void setEventsPtr(ModelEvents *events);
  [[nodiscard]] const QStringList &getIds() const;
  [[nodiscard]] const QStringList &getNames() const;
  QString setName(const QString &id, const QString &name);
  [[nodiscard]] QString getName(const QString &id) const;
  void setExpression(const QString &id, const QString &expr);
  [[nodiscard]] QString getExpression(const QString &id) const;
  QString add(const QString &name);
  void remove(const QString &id);
  [[nodiscard]] const SpatialCoordinates &getSpatialCoordinates() const;
  void setSpatialCoordinates(SpatialCoordinates coords);
  [[nodiscard]] std::vector<IdName>
  getSymbols(const QStringList &compartments = {}) const;
  [[nodiscard]] std::vector<IdNameValue> getGlobalConstants() const;
  [[nodiscard]] std::vector<IdNameExpr> getNonConstantParameters() const;
  [[nodiscard]] bool getHasUnsavedChanges() const;
  void setHasUnsavedChanges(bool unsavedChanges);
};

} // namespace sme::model
