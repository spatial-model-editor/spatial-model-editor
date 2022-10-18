#pragma once

#include "sme/geometry.hpp"
#include "sme/model_species_types.hpp"
#include <QColor>
#include <QStringList>
#include <map>
#include <optional>
#include <string>

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
  [[nodiscard]] std::vector<double>
  getSampledFieldConcentrationFromSBML(const QString &id) const;
  bool hasUnsavedChanges{false};

public:
  ModelSpecies();
  ModelSpecies(libsbml::Model *model, const ModelCompartments *compartments,
               const ModelGeometry *geometry, const ModelParameters *parameters,
               simulate::SimulationData *data, Settings *annotation);
  void setReactionsPtr(ModelReactions *reactions);
  void setSimulationDataPtr(simulate::SimulationData *data);
  [[nodiscard]] bool containsNonSpatialReactiveSpecies() const;
  QString add(const QString &name, const QString &compartmentId);
  void remove(const QString &id);
  QString setName(const QString &id, const QString &name);
  [[nodiscard]] QString getName(const QString &id) const;
  void updateCompartmentGeometry(const QString &compartmentId);
  void setCompartment(const QString &id, const QString &compartmentId);
  [[nodiscard]] QString getCompartment(const QString &id) const;
  [[nodiscard]] QStringList getIds(const QString &compartmentId) const;
  [[nodiscard]] QStringList getNames(const QString &compartmentId) const;
  void setIsSpatial(const QString &id, bool isSpatial);
  [[nodiscard]] bool getIsSpatial(const QString &id) const;
  void setDiffusionConstant(const QString &id, double diffusionConstant);
  [[nodiscard]] double getDiffusionConstant(const QString &id) const;
  [[nodiscard]] ConcentrationType
  getInitialConcentrationType(const QString &id) const;
  void setInitialConcentration(const QString &id, double concentration);
  [[nodiscard]] double getInitialConcentration(const QString &id) const;
  void setAnalyticConcentration(const QString &id,
                                const QString &analyticExpression);
  void setFieldConcAnalytic(
      geometry::Field &field, const std::string &expr,
      const std::map<std::string, double, std::less<>> &substitutions = {});
  [[nodiscard]] QString getAnalyticConcentration(const QString &id) const;
  void updateAllAnalyticConcentrations();
  void
  setSampledFieldConcentration(const QString &id,
                               const std::vector<double> &concentrationArray);
  [[nodiscard]] std::vector<double>
  getSampledFieldConcentration(const QString &id,
                               bool maskAndInvertY = false) const;
  [[nodiscard]] std::vector<QImage>
  getConcentrationImages(const QString &id) const;
  void setColour(const QString &id, QRgb colour);
  [[nodiscard]] QRgb getColour(const QString &id) const;
  void setIsConstant(const QString &id, bool constant);
  [[nodiscard]] bool getIsConstant(const QString &id) const;
  [[nodiscard]] bool isReactive(const QString &id) const;
  void removeInitialAssignments();
  [[nodiscard]] QString
  getSampledFieldInitialAssignment(const QString &id) const;
  geometry::Field *getField(const QString &id);
  [[nodiscard]] const geometry::Field *getField(const QString &id) const;
  [[nodiscard]] bool getHasUnsavedChanges() const;
  void setHasUnsavedChanges(bool unsavedChanges);
};

} // namespace sme::model
