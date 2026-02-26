// SBML sId and Name utility functions

#pragma once

#include "sme/geometry.hpp"
#include "sme/model_types.hpp"
#include <QPointF>
#include <QRgb>
#include <QStringList>
#include <QVector>
#include <map>
#include <memory>
#include <optional>

namespace libsbml {
class Model;
}

namespace sme {

namespace simulate {
class SimulationData;
}

namespace model {
class ModelGeometry;
class ModelMembranes;
class ModelSpecies;
class ModelReactions;
class ModelUnits;

/**
 * @brief SBML compartment manager plus geometry-backed compartment objects.
 */
class ModelCompartments {
private:
  friend class ModelGeometry;
  QStringList ids;
  QStringList names;
  QVector<QRgb> colors;
  std::vector<std::unique_ptr<geometry::Compartment>> compartments;
  libsbml::Model *sbmlModel = nullptr;
  ModelGeometry *modelGeometry = nullptr;
  ModelMembranes *modelMembranes = nullptr;
  ModelSpecies *modelSpecies = nullptr;
  ModelReactions *modelReactions = nullptr;
  const ModelUnits *modelUnits{nullptr};
  simulate::SimulationData *simulationData = nullptr;
  bool hasUnsavedChanges{false};
  std::map<std::string, double, std::less<>> initialCompartmentSizes{};
  void updateGeometryImageColor(QRgb oldColor, QRgb newColor);

public:
  /**
   * @brief Construct empty compartment model.
   */
  ModelCompartments();
  /**
   * @brief Construct compartment model from SBML model.
   */
  ModelCompartments(libsbml::Model *model, ModelMembranes *membranes,
                    const ModelUnits *units, simulate::SimulationData *data);
  /**
   * @brief Set geometry dependency pointer.
   */
  void setGeometryPtr(ModelGeometry *geometry);
  /**
   * @brief Set species dependency pointer.
   */
  void setSpeciesPtr(ModelSpecies *species);
  /**
   * @brief Set reactions dependency pointer.
   */
  void setReactionsPtr(ModelReactions *reactions);
  /**
   * @brief Set simulation-data dependency pointer.
   */
  void setSimulationDataPtr(simulate::SimulationData *data);
  /**
   * @brief Compartment ids.
   */
  [[nodiscard]] const QStringList &getIds() const;
  /**
   * @brief Compartment names.
   */
  [[nodiscard]] const QStringList &getNames() const;
  /**
   * @brief Compartment display colors.
   */
  [[nodiscard]] const QVector<QRgb> &getColors() const;
  /**
   * @brief Add compartment.
   */
  QString add(const QString &name);
  /**
   * @brief Remove compartment.
   */
  bool remove(const QString &id);
  /**
   * @brief Get compartment name.
   */
  [[nodiscard]] QString getName(const QString &id) const;
  /**
   * @brief Set compartment name.
   */
  QString setName(const QString &id, const QString &name);
  /**
   * @brief Get interior points used for meshing.
   */
  [[nodiscard]] std::optional<std::vector<QPointF>>
  getInteriorPoints(const QString &id) const;
  /**
   * @brief Set interior points used for meshing.
   */
  void setInteriorPoints(const QString &id, const std::vector<QPointF> &points);
  /**
   * @brief Set compartment display color.
   */
  void setColor(const QString &id, QRgb color);
  /**
   * @brief Get compartment display color.
   */
  [[nodiscard]] QRgb getColor(const QString &id) const;
  /**
   * @brief Find compartment id from display color.
   */
  [[nodiscard]] QString getIdFromColor(QRgb color) const;
  /**
   * @brief Geometry-backed compartment objects.
   */
  [[nodiscard]] const std::vector<std::unique_ptr<geometry::Compartment>> &
  getCompartments() const;
  /**
   * @brief Mutable geometry-backed compartment object.
   */
  geometry::Compartment *getCompartment(const QString &id);
  /**
   * @brief Immutable geometry-backed compartment object.
   */
  [[nodiscard]] const geometry::Compartment *
  getCompartment(const QString &id) const;
  /**
   * @brief Compartment size in model units.
   */
  [[nodiscard]] double getSize(const QString &id) const;
  /**
   * @brief Initial compartment sizes captured on import.
   */
  [[nodiscard]] const std::map<std::string, double, std::less<>> &
  getInitialCompartmentSizes() const;
  /**
   * @brief Clear all compartments.
   */
  void clear();
  /**
   * @brief Unsaved state flag.
   */
  [[nodiscard]] bool getHasUnsavedChanges() const;
  /**
   * @brief Set unsaved state flag.
   */
  void setHasUnsavedChanges(bool unsavedChanges);
};

} // namespace model

} // namespace sme
