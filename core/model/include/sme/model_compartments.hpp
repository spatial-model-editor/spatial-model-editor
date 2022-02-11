// SBML sId and Name utility functions

#pragma once

#include "sme/geometry.hpp"
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

class ModelCompartments {
private:
  QStringList ids;
  QStringList names;
  QVector<QRgb> colours;
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

public:
  ModelCompartments();
  ModelCompartments(libsbml::Model *model, ModelMembranes *membranes,
                    const ModelUnits *units, simulate::SimulationData *data);
  void setGeometryPtr(ModelGeometry *geometry);
  void setSpeciesPtr(ModelSpecies *species);
  void setReactionsPtr(ModelReactions *reactions);
  void setSimulationDataPtr(simulate::SimulationData *data);
  [[nodiscard]] const QStringList &getIds() const;
  [[nodiscard]] const QStringList &getNames() const;
  [[nodiscard]] const QVector<QRgb> &getColours() const;
  QString add(const QString &name);
  bool remove(const QString &id);
  [[nodiscard]] QString getName(const QString &id) const;
  QString setName(const QString &id, const QString &name);
  [[nodiscard]] std::optional<std::vector<QPointF>>
  getInteriorPoints(const QString &id) const;
  void setInteriorPoints(const QString &id, const std::vector<QPointF> &points);
  void setColour(const QString &id, QRgb colour);
  [[nodiscard]] QRgb getColour(const QString &id) const;
  [[nodiscard]] QString getIdFromColour(QRgb colour) const;
  [[nodiscard]] const std::vector<std::unique_ptr<geometry::Compartment>> &
  getCompartments() const;
  geometry::Compartment *getCompartment(const QString &id);
  [[nodiscard]] const geometry::Compartment *
  getCompartment(const QString &id) const;
  [[nodiscard]] double getSize(const QString &id) const;
  [[nodiscard]] const std::map<std::string, double, std::less<>> &
  getInitialCompartmentSizes() const;
  void clear();
  [[nodiscard]] bool getHasUnsavedChanges() const;
  void setHasUnsavedChanges(bool unsavedChanges);
};

} // namespace model

} // namespace sme
