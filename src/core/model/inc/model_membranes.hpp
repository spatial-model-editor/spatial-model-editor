// Membrane compartments

#pragma once

#include <QColor>
#include <QImage>
#include <QStringList>
#include <memory>
#include <optional>
#include <string>
#include <vector>

#include "geometry.hpp"

namespace libsbml {
class Model;
}

namespace model {

class ImageMembranePixels;
using QPointPair = std::pair<QPoint, QPoint>;

class ModelMembranes {
private:
  QStringList ids;
  QStringList names;
  QStringList compIds;
  std::vector<geometry::Membrane> membranes;
  std::unique_ptr<ImageMembranePixels> membranePixels;
  std::vector<std::pair<std::string, std::pair<QRgb, QRgb>>> idColourPairs;

public:
  const QStringList &getIds() const;
  const QStringList &getNames() const;
  QString getName(const QString &id) const;
  const std::vector<geometry::Membrane> &getMembranes() const;
  const geometry::Membrane *getMembrane(const QString &id) const;
  const std::vector<std::pair<std::string, std::pair<QRgb, QRgb>>> &
  getIdColourPairs() const;
  void clear();
  void updateCompartmentNames(const QStringList &compartmentNames,
                              const libsbml::Model *model);
  void updateCompartments(
      const std::vector<std::unique_ptr<geometry::Compartment>> &compartments);
  void updateCompartmentImage(const QImage &img);
  void importMembraneIdsAndNames(const libsbml::Model *model);
  void exportToSBML(libsbml::Model *model);
  explicit ModelMembranes();
  ModelMembranes(ModelMembranes &&) noexcept;
  ModelMembranes &operator=(ModelMembranes &&) noexcept;
  ModelMembranes &operator=(const ModelMembranes &) = delete;
  ModelMembranes(const ModelMembranes &) = delete;
  ~ModelMembranes();
};

} // namespace model
