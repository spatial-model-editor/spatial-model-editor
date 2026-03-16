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

/**
 * @brief SBML species manager with spatial field data.
 */
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
  void removeInvalidCrossDiffusionConstants();

public:
  /**
   * @brief Construct empty species model.
   */
  ModelSpecies();
  /**
   * @brief Construct species model from SBML model and dependencies.
   * @param model SBML model pointer.
   * @param compartments Compartment manager.
   * @param geometry Geometry manager.
   * @param parameters Parameter manager.
   * @param functions Function manager.
   * @param data Simulation data storage.
   * @param annotation Settings annotation.
   */
  ModelSpecies(libsbml::Model *model, const ModelCompartments *compartments,
               const ModelGeometry *geometry, const ModelParameters *parameters,
               const ModelFunctions *functions, simulate::SimulationData *data,
               Settings *annotation);
  /**
   * @brief Set reactions dependency pointer.
   * @param reactions Reaction manager.
   */
  void setReactionsPtr(ModelReactions *reactions);
  /**
   * @brief Set simulation-data dependency pointer.
   * @param data Simulation data storage.
   */
  void setSimulationDataPtr(simulate::SimulationData *data);
  /**
   * @brief Returns ``true`` if any reactive species is non-spatial.
   * @returns ``true`` if non-spatial reactive species exist.
   */
  [[nodiscard]] bool containsNonSpatialReactiveSpecies() const;
  /**
   * @brief Add species to compartment.
   * @param name Species display name.
   * @param compartmentId Target compartment id.
   * @returns New species id.
   */
  QString add(const QString &name, const QString &compartmentId);
  /**
   * @brief Remove species.
   * @param id Species id.
   */
  void remove(const QString &id);
  /**
   * @brief Set species name.
   * @param id Species id.
   * @param name New species name.
   * @returns Final (possibly uniquified) name.
   */
  QString setName(const QString &id, const QString &name);
  /**
   * @brief Get species name.
   * @param id Species id.
   * @returns Species name.
   */
  [[nodiscard]] QString getName(const QString &id) const;
  /**
   * @brief Rebuild field geometry after compartment geometry changes.
   * @param compartmentId Compartment id.
   */
  void updateCompartmentGeometry(const QString &compartmentId);
  /**
   * @brief Move species to another compartment.
   * @param id Species id.
   * @param compartmentId Target compartment id.
   */
  void setCompartment(const QString &id, const QString &compartmentId);
  /**
   * @brief Get species compartment id.
   * @param id Species id.
   * @returns Compartment id.
   */
  [[nodiscard]] QString getCompartment(const QString &id) const;
  /**
   * @brief Species ids in a compartment.
   * @param compartmentId Compartment id.
   * @returns Species ids in compartment.
   */
  [[nodiscard]] QStringList getIds(const QString &compartmentId) const;
  /**
   * @brief Species names in a compartment.
   * @param compartmentId Compartment id.
   * @returns Species names in compartment.
   */
  [[nodiscard]] QStringList getNames(const QString &compartmentId) const;
  /**
   * @brief Set whether species is spatial.
   * @param id Species id.
   * @param isSpatial Spatial flag.
   */
  void setIsSpatial(const QString &id, bool isSpatial);
  /**
   * @brief Returns whether species is spatial.
   * @param id Species id.
   * @returns Spatial flag.
   */
  [[nodiscard]] bool getIsSpatial(const QString &id) const;
  /**
   * @brief Set uniform diffusion constant.
   * @param id Species id.
   * @param diffusionConstant Diffusion constant value.
   */
  void setDiffusionConstant(const QString &id, double diffusionConstant);
  /**
   * @brief Get uniform diffusion constant.
   * @param id Species id.
   * @returns Diffusion constant.
   */
  [[nodiscard]] double getDiffusionConstant(const QString &id) const;
  /**
   * @brief Set species storage coefficient.
   * @param id Species id.
   * @param storageValue Storage value.
   */
  void setStorage(const QString &id, double storageValue);
  /**
   * @brief Get species storage coefficient.
   * @param id Species id.
   * @returns Storage value.
   */
  [[nodiscard]] double getStorage(const QString &id) const;
  /**
   * @brief Representation type of diffusion constant data.
   * @param id Species id.
   * @returns Diffusion data representation type.
   */
  [[nodiscard]] SpatialDataType
  getDiffusionConstantType(const QString &id) const;
  /**
   * @brief Representation type of initial concentration data.
   * @param id Species id.
   * @returns Initial concentration representation type.
   */
  [[nodiscard]] SpatialDataType
  getInitialConcentrationType(const QString &id) const;
  /**
   * @brief Set uniform initial concentration.
   * @param id Species id.
   * @param concentration Initial concentration value.
   */
  void setInitialConcentration(const QString &id, double concentration);
  /**
   * @brief Get uniform initial concentration.
   * @param id Species id.
   * @returns Initial concentration value.
   */
  [[nodiscard]] double getInitialConcentration(const QString &id) const;
  /**
   * @brief Set analytic initial concentration expression.
   * @param id Species id.
   * @param analyticExpression Analytic concentration expression.
   */
  void setAnalyticConcentration(const QString &id,
                                const QString &analyticExpression);
  /**
   * @brief Apply analytic concentration expression to a field.
   * @param field Field to update.
   * @param expr Analytic expression.
   * @param substitutions Constant substitutions.
   */
  void setFieldConcAnalytic(
      geometry::Field &field, const std::string &expr,
      const std::map<std::string, double, std::less<>> &substitutions = {});
  /**
   * @brief Get analytic concentration expression.
   * @param id Species id.
   * @returns Analytic concentration expression.
   */
  [[nodiscard]] QString getAnalyticConcentration(const QString &id) const;
  /**
   * @brief Recompute all analytic concentrations.
   */
  void updateAllAnalyticConcentrations();
  /**
   * @brief Set sampled-field concentration array.
   * @param id Species id.
   * @param concentrationArray Concentration values in image order.
   */
  void
  setSampledFieldConcentration(const QString &id,
                               const std::vector<double> &concentrationArray);
  /**
   * @brief Get sampled-field concentration array.
   * @param id Species id.
   * @param maskAndInvertY If ``true``, apply mask and Y inversion.
   * @returns Concentration values in image order.
   */
  [[nodiscard]] std::vector<double>
  getSampledFieldConcentration(const QString &id,
                               bool maskAndInvertY = false) const;
  /**
   * @brief Set sampled-field diffusion constant array.
   * @param id Species id.
   * @param diffusionConstantArray Diffusion values in image order.
   */
  void setSampledFieldDiffusionConstant(
      const QString &id, const std::vector<double> &diffusionConstantArray);
  /**
   * @brief Get sampled-field diffusion constant array.
   * @param id Species id.
   * @param maskAndInvertY If ``true``, apply mask and Y inversion.
   * @returns Diffusion values in image order.
   */
  [[nodiscard]] std::vector<double>
  getSampledFieldDiffusionConstant(const QString &id,
                                   bool maskAndInvertY = false) const;
  /**
   * @brief Validate analytic diffusion expression.
   * @param analyticExpression Expression to validate.
   * @returns ``true`` if expression is valid.
   */
  [[nodiscard]] bool
  isValidAnalyticDiffusionExpression(const QString &analyticExpression) const;
  /**
   * @brief Set analytic diffusion constant expression.
   * @param id Species id.
   * @param analyticExpression Analytic diffusion expression.
   */
  void setAnalyticDiffusionConstant(const QString &id,
                                    const QString &analyticExpression);
  /**
   * @brief Get analytic diffusion constant expression.
   * @param id Species id.
   * @returns Analytic diffusion expression.
   */
  [[nodiscard]] QString getAnalyticDiffusionConstant(const QString &id) const;
  /**
   * @brief Validate cross-diffusion expression for species pair.
   * @param id Primary species id.
   * @param expression Cross-diffusion expression.
   * @returns ``true`` if expression is valid.
   */
  [[nodiscard]] bool
  isValidCrossDiffusionExpression(const QString &id,
                                  const QString &expression) const;
  /**
   * @brief Set cross-diffusion expression for species pair.
   * @param id Primary species id.
   * @param otherId Coupled species id.
   * @param expression Cross-diffusion expression.
   */
  void setCrossDiffusionConstant(const QString &id, const QString &otherId,
                                 const QString &expression);
  /**
   * @brief Remove cross-diffusion expression for species pair.
   * @param id Primary species id.
   * @param otherId Coupled species id.
   */
  void removeCrossDiffusionConstant(const QString &id, const QString &otherId);
  /**
   * @brief Get cross-diffusion expression for species pair.
   * @param id Primary species id.
   * @param otherId Coupled species id.
   * @returns Cross-diffusion expression.
   */
  [[nodiscard]] QString getCrossDiffusionConstant(const QString &id,
                                                  const QString &otherId) const;
  /**
   * @brief Get all cross-diffusion expressions for a species.
   * @param id Species id.
   * @returns Mapping from coupled species id to expression.
   */
  [[nodiscard]] std::map<std::string, std::string>
  getCrossDiffusionConstants(const QString &id) const;
  /**
   * @brief Concentration images for species.
   * @param id Species id.
   * @returns Concentration image stack.
   */
  [[nodiscard]] common::ImageStack
  getConcentrationImages(const QString &id) const;
  /**
   * @brief Set species display color.
   * @param id Species id.
   * @param color New display color.
   */
  void setColor(const QString &id, QRgb color);
  /**
   * @brief Get species display color.
   * @param id Species id.
   * @returns Species display color.
   */
  [[nodiscard]] QRgb getColor(const QString &id) const;
  /**
   * @brief Set constant/non-constant species flag.
   * @param id Species id.
   * @param constant Constant flag.
   */
  void setIsConstant(const QString &id, bool constant);
  /**
   * @brief Get constant/non-constant species flag.
   * @param id Species id.
   * @returns Constant flag.
   */
  [[nodiscard]] bool getIsConstant(const QString &id) const;
  /**
   * @brief Returns whether species is a scalar constant.
   * @param id Species id.
   * @returns ``true`` if species is constant and non-spatial.
   */
  [[nodiscard]] bool isScalarConstantSpecies(const QString &id) const;
  /**
   * @brief Returns whether species should be present in simulation fields.
   * @param id Species id.
   * @returns ``true`` if species should appear in simulation state/results.
   */
  [[nodiscard]] bool isSimulatedSpecies(const QString &id) const;
  /**
   * @brief Returns whether species participates in any reaction.
   * @param id Species id.
   * @returns ``true`` if species is reactive.
   */
  [[nodiscard]] bool isReactive(const QString &id) const;
  /**
   * @brief Remove all species initial assignments.
   */
  void removeInitialAssignments();
  /**
   * @brief Get sampled-field initial-assignment expression.
   * @param id Species id.
   * @returns Initial-assignment expression.
   */
  [[nodiscard]] QString
  getSampledFieldInitialAssignment(const QString &id) const;
  /**
   * @brief Mutable field object for species id.
   * @param id Species id.
   * @returns Mutable field pointer or ``nullptr``.
   */
  geometry::Field *getField(const QString &id);
  /**
   * @brief Immutable field object for species id.
   * @param id Species id.
   * @returns Immutable field pointer or ``nullptr``.
   */
  [[nodiscard]] const geometry::Field *getField(const QString &id) const;
  /**
   * @brief Unsaved state flag.
   * @returns Unsaved state flag.
   */
  [[nodiscard]] bool getHasUnsavedChanges() const;
  /**
   * @brief Set unsaved state flag.
   * @param unsavedChanges New unsaved state.
   */
  void setHasUnsavedChanges(bool unsavedChanges);
};

} // namespace sme::model
