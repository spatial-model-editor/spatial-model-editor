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
  explicit ModelParameters(libsbml::Model *model,
                           ModelEvents *events = nullptr);
  const QStringList &getIds() const;
  const QStringList &getNames() const;
  QString setName(const QString &id, const QString &name);
  QString getName(const QString &id) const;
  void setExpression(const QString &id, const QString &expr);
  QString getExpression(const QString &id) const;
  QString add(const QString &name);
  void remove(const QString &id);
  const SpatialCoordinates &getSpatialCoordinates() const;
  void setSpatialCoordinates(SpatialCoordinates coords);
  std::vector<IdName> getSymbols(const QStringList &compartments = {}) const;
  std::vector<IdNameValue> getGlobalConstants() const;
  std::vector<IdNameExpr> getNonConstantParameters() const;
  bool getHasUnsavedChanges() const;
  void setHasUnsavedChanges(bool unsavedChanges);
};

} // namespace sme::model
