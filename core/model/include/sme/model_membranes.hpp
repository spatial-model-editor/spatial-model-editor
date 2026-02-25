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

/**
 * @brief SBML membrane manager.
 */
class ModelMembranes {
private:
  QStringList ids{};
  QStringList names{};
  QStringList compIds{};
  std::vector<geometry::Membrane> membranes{};
  std::unique_ptr<ImageMembranePixels> membranePixels;
  std::vector<std::pair<std::string, std::pair<QRgb, QRgb>>> idColorPairs{};
  libsbml::Model *sbmlModel{nullptr};
  bool hasUnsavedChanges{false};

public:
  /**
   * @brief Membrane ids.
   */
  [[nodiscard]] const QStringList &getIds() const;
  /**
   * @brief Membrane names.
   */
  [[nodiscard]] const QStringList &getNames() const;
  /**
   * @brief Set membrane display name.
   */
  QString setName(const QString &id, const QString &name);
  /**
   * @brief Get membrane display name.
   */
  [[nodiscard]] QString getName(const QString &id) const;
  /**
   * @brief Geometry membrane objects.
   */
  [[nodiscard]] const std::vector<geometry::Membrane> &getMembranes() const;
  /**
   * @brief Get membrane by id.
   */
  [[nodiscard]] const geometry::Membrane *getMembrane(const QString &id) const;
  /**
   * @brief Membrane id and adjacent compartment color pairs.
   */
  [[nodiscard]] const std::vector<
      std::pair<std::string, std::pair<QRgb, QRgb>>> &
  getIdColorPairs() const;
  /**
   * @brief Membrane size in model units.
   */
  [[nodiscard]] double getSize(const QString &id) const;
  /**
   * @brief Update cached compartment-name text.
   */
  void updateCompartmentNames(const QStringList &compartmentNames);
  /**
   * @brief Recompute membranes from compartment geometry.
   */
  void updateCompartments(
      const std::vector<std::unique_ptr<geometry::Compartment>> &compartments);
  /**
   * @brief Recompute membrane pixels from geometry images.
   */
  void updateCompartmentImages(const common::ImageStack &imgs);
  /**
   * @brief Replace a compartment color in membrane definitions.
   */
  void updateGeometryImageColor(QRgb oldColor, QRgb newColor);
  /**
   * @brief Import membrane ids/names from SBML annotations.
   */
  void importMembraneIdsAndNames();
  /**
   * @brief Export membrane data to SBML.
   */
  void exportToSBML(const common::VolumeF &voxelSize);
  /**
   * @brief Construct membrane model.
   */
  explicit ModelMembranes(libsbml::Model *model = nullptr);
  /**
   * @brief Destructor.
   */
  ~ModelMembranes();
  /**
   * @brief Unsaved state flag.
   */
  [[nodiscard]] bool getHasUnsavedChanges() const;
  /**
   * @brief Set unsaved state flag.
   */
  void setHasUnsavedChanges(bool unsavedChanges);
};

} // namespace sme::model
