// SBML sId and Name utility functions

#pragma once

#include <QPointF>
#include <QRgb>
#include <QStringList>
#include <QVector>
#include <optional>

#include "geometry.hpp"

namespace libsbml {
class Model;
}

namespace model {
class ModelGeometry;
class ModelMembranes;
class ModelSpecies;

class ModelCompartments {
 private:
  QStringList ids;
  QStringList names;
  QVector<QRgb> colours;
  std::vector<geometry::Compartment> compartments;
  libsbml::Model *sbmlModel = nullptr;
  ModelGeometry *modelGeometry = nullptr;
  ModelMembranes *modelMembranes = nullptr;
  ModelSpecies *modelSpecies = nullptr;

 public:
  ModelCompartments();
  ModelCompartments(libsbml::Model *model, ModelGeometry *geometry,
                    ModelMembranes *membranes, ModelSpecies *species);
  const QStringList &getIds() const;
  const QStringList &getNames() const;
  const QVector<QRgb> &getColours() const;
  QString add(const QString &name);
  bool remove(const QString &id);
  QString getName(const QString &id) const;
  QString setName(const QString &id, const QString &name);
  std::optional<QPointF> getInteriorPoint(const QString &id) const;
  void setInteriorPoint(const QString &id, const QPointF &point);
  void setColour(const QString &id, QRgb colour);
  QRgb getColour(const QString &id) const;
  QString getIdFromColour(QRgb colour) const;
  geometry::Compartment *getCompartment(const QString &id);
  const geometry::Compartment *getCompartment(const QString &id) const;
  void clear();
};

}  // namespace sbml
