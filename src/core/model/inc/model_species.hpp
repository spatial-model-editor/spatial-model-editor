// SBML sId and Name utility functions

#pragma once

#include <QColor>
#include <QStringList>
#include <map>
#include <optional>
#include <string>
#include "geometry.hpp"

namespace libsbml {
class Model;
class Species;
} // namespace libsbml

namespace sme::simulate {
class SimulationData;
}

namespace sme::model {

class ModelCompartments;
class ModelGeometry;
class ModelParameters;
class ModelReactions;
struct Settings;

class ModelSpecies {
private:
  QStringList ids;
  QStringList names;
  QStringList compartmentIds;
  std::vector<geometry::Field> fields;
  libsbml::Model *sbmlModel = nullptr;
  const ModelCompartments *modelCompartments = nullptr;
  const ModelGeometry *modelGeometry = nullptr;
  const ModelParameters *modelParameters = nullptr;
  ModelReactions *modelReactions = nullptr;
  simulate::SimulationData *simulationData = nullptr;
  Settings *sbmlAnnotation = nullptr;
  void removeInitialAssignment(const QString &id);
  std::vector<double>
  getSampledFieldConcentrationFromSBML(const QString &id) const;
  bool hasUnsavedChanges{false};

public:
  ModelSpecies();
  ModelSpecies(libsbml::Model *model, const ModelCompartments *compartments,
               const ModelGeometry *geometry, const ModelParameters *parameters,
               ModelReactions *reactions, simulate::SimulationData *data,
               Settings *annotation);
  QString add(const QString &name, const QString &compartmentId);
  void remove(const QString &id);
  QString setName(const QString &id, const QString &name);
  QString getName(const QString &id) const;
  void updateCompartmentGeometry(const QString &compartmentId);
  void setCompartment(const QString &id, const QString &compartmentId);
  QString getCompartment(const QString &id) const;
  QStringList getIds(const QString &compartmentId) const;
  QStringList getNames(const QString &compartmentId) const;
  void setIsSpatial(const QString &id, bool isSpatial);
  bool getIsSpatial(const QString &id) const;
  void setDiffusionConstant(const QString &id, double diffusionConstant);
  double getDiffusionConstant(const QString &id) const;
  void setInitialConcentration(const QString &id, double concentration);
  double getInitialConcentration(const QString &id) const;
  void setAnalyticConcentration(const QString &id,
                                const QString &analyticExpression);
  void
  setFieldConcAnalytic(geometry::Field &field, const std::string &expr,
                       const std::map<std::string, double, std::less<>> &substitutions = {});
  QString getAnalyticConcentration(const QString &id) const;
  void
  setSampledFieldConcentration(const QString &id,
                               const std::vector<double> &concentrationArray);
  std::vector<double> getSampledFieldConcentration(const QString &id) const;
  QImage getConcentrationImage(const QString &id) const;
  void setColour(const QString &id, QRgb colour);
  QRgb getColour(const QString &id) const;
  void setIsConstant(const QString &id, bool constant);
  bool getIsConstant(const QString &id) const;
  bool isReactive(const QString &id) const;
  void removeInitialAssignments();
  QString getSampledFieldInitialAssignment(const QString &id) const;
  geometry::Field *getField(const QString &id);
  const geometry::Field *getField(const QString &id) const;
  bool getHasUnsavedChanges() const;
  void setHasUnsavedChanges(bool unsavedChanges);
};

} // namespace sme::model
