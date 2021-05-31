// SBML sId and Name utility functions

#pragma once

#include <QPointF>
#include <QRgb>
#include <QStringList>
#include <QVector>
#include <memory>
#include <optional>

#include "geometry.hpp"

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
  simulate::SimulationData *simulationData = nullptr;
  bool hasUnsavedChanges{false};

public:
  ModelCompartments();
  ModelCompartments(libsbml::Model *model, ModelGeometry *geometry,
                    ModelMembranes *membranes, ModelSpecies *species,
                    ModelReactions *reactions, simulate::SimulationData *data);
  const QStringList &getIds() const;
  const QStringList &getNames() const;
  const QVector<QRgb> &getColours() const;
  QString add(const QString &name);
  bool remove(const QString &id);
  QString getName(const QString &id) const;
  QString setName(const QString &id, const QString &name);
  std::optional<std::vector<QPointF>>
  getInteriorPoints(const QString &id) const;
  void setInteriorPoints(const QString &id, const std::vector<QPointF> &points);
  void setColour(const QString &id, QRgb colour);
  QRgb getColour(const QString &id) const;
  QString getIdFromColour(QRgb colour) const;
  const std::vector<std::unique_ptr<geometry::Compartment>> &
  getCompartments() const;
  geometry::Compartment *getCompartment(const QString &id);
  const geometry::Compartment *getCompartment(const QString &id) const;
  double getSize(const QString& id) const;
  void clear();
  bool getHasUnsavedChanges() const;
  void setHasUnsavedChanges(bool unsavedChanges);

};

} // namespace model

} // namespace sme
