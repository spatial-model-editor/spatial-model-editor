// Membrane compartments

#pragma once

#include "sme/geometry.hpp"
#include <QColor>
#include <QImage>
#include <QStringList>
#include <memory>
#include <optional>
#include <string>
#include <vector>

namespace libsbml {
class Model;
}

namespace sme::model {

class ImageMembranePixels;
using QPointPair = std::pair<QPoint, QPoint>;

class ModelMembranes {
private:
  QStringList ids{};
  QStringList names{};
  QStringList compIds{};
  std::vector<geometry::Membrane> membranes{};
  std::unique_ptr<ImageMembranePixels> membranePixels;
  std::vector<std::pair<std::string, std::pair<QRgb, QRgb>>> idColourPairs{};
  libsbml::Model *sbmlModel{nullptr};
  bool hasUnsavedChanges{false};

public:
  [[nodiscard]] const QStringList &getIds() const;
  [[nodiscard]] const QStringList &getNames() const;
  QString setName(const QString &id, const QString &name);
  [[nodiscard]] QString getName(const QString &id) const;
  [[nodiscard]] const std::vector<geometry::Membrane> &getMembranes() const;
  [[nodiscard]] const geometry::Membrane *getMembrane(const QString &id) const;
  [[nodiscard]] const std::vector<std::pair<std::string, std::pair<QRgb, QRgb>>>
      &getIdColourPairs() const;
  [[nodiscard]] double getSize(const QString &id) const;
  void updateCompartmentNames(const QStringList &compartmentNames);
  void updateCompartments(
      const std::vector<std::unique_ptr<geometry::Compartment>> &compartments);
  void updateCompartmentImages(const std::vector<QImage> &imgs);
  void importMembraneIdsAndNames();
  void exportToSBML(const geometry::VSizeF &voxelSize);
  explicit ModelMembranes(libsbml::Model *model = nullptr);
  ModelMembranes(ModelMembranes &&) noexcept;
  ModelMembranes &operator=(ModelMembranes &&) noexcept;
  ModelMembranes &operator=(const ModelMembranes &) = delete;
  ModelMembranes(const ModelMembranes &) = delete;
  ~ModelMembranes();
  [[nodiscard]] bool getHasUnsavedChanges() const;
  void setHasUnsavedChanges(bool unsavedChanges);
};

} // namespace sme::model
