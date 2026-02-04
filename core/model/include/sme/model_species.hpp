#pragma once

#include "sme/geometry.hpp"
#include "sme/model_types.hpp"
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
class ModelFunctions;
struct Settings;

class ModelSpecies {
private:
  enum class AnalyticValueType { Concentration, DiffusionConstant };
  QStringList ids;
  QStringList names;
  QStringList compartmentIds;
  std::vector<geometry::Field> fields;
  libsbml::Model *sbmlModel = nullptr;
  const ModelCompartments *modelCompartments = nullptr;
  const ModelGeometry *modelGeometry = nullptr;
  const ModelParameters *modelParameters = nullptr;
  ModelReactions *modelReactions = nullptr;
  const ModelFunctions *modelFunctions = nullptr;
  simulate::SimulationData *simulationData = nullptr;
  Settings *sbmlAnnotation = nullptr;
  void removeInitialAssignment(const QString &id);
  [[nodiscard]] std::vector<double>
  getSampledFieldFromSBML(const QString &id) const;
  bool hasUnsavedChanges{false};
  void setFieldAnalyticValues(
      geometry::Field &field, const std::string &expr,
      AnalyticValueType valueType,
      const std::map<std::string, double, std::less<>> &substitutions);
  void setFieldDiffAnalytic(
      geometry::Field &field, const std::string &expr,
      const std::map<std::string, double, std::less<>> &substitutions = {});

public:
  ModelSpecies();
  ModelSpecies(libsbml::Model *model, const ModelCompartments *compartments,
               const ModelGeometry *geometry, const ModelParameters *parameters,
               const ModelFunctions *functions, simulate::SimulationData *data,
               Settings *annotation);
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
  [[nodiscard]] SpatialDataType
  getDiffusionConstantType(const QString &id) const;
  [[nodiscard]] SpatialDataType
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
  void setSampledFieldDiffusionConstant(
      const QString &id, const std::vector<double> &diffusionConstantArray);
  [[nodiscard]] std::vector<double>
  getSampledFieldDiffusionConstant(const QString &id,
                                   bool maskAndInvertY = false) const;
  [[nodiscard]] bool
  isValidAnalyticDiffusionExpression(const QString &analyticExpression) const;
  void setAnalyticDiffusionConstant(const QString &id,
                                    const QString &analyticExpression);
  [[nodiscard]] QString getAnalyticDiffusionConstant(const QString &id) const;
  [[nodiscard]] common::ImageStack
  getConcentrationImages(const QString &id) const;
  void setColor(const QString &id, QRgb color);
  [[nodiscard]] QRgb getColor(const QString &id) const;
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
