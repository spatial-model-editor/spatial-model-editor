// SBML Model Geometry
//   - import geometry from SBML sampled field geometry

#pragma once

#include "sme/geometry.hpp"
#include "sme/image_stack.hpp"
#include <QImage>
#include <QRgb>
#include <memory>
#include <optional>
#include <string>
#include <vector>

namespace libsbml {
class Model;
}

namespace sme {

namespace mesh {
class Mesh2d;
class Mesh3d;
} // namespace mesh

namespace model {

class ModelCompartments;
class ModelMembranes;
class ModelUnits;
struct Settings;

/**
 * @brief SBML geometry manager and mesh owner.
 */
class ModelGeometry {
private:
  common::VoxelF physicalOrigin{};
  common::VolumeF physicalSize{};
  common::VolumeF voxelSize{1.0, 1.0, 1.0};
  int numDimensions{3};
  common::ImageStack images;
  std::unique_ptr<mesh::Mesh2d> mesh;
  std::unique_ptr<mesh::Mesh3d> mesh3d;
  bool isValid{false};
  bool isMeshValid{false};
  bool hasImage{false};
  libsbml::Model *sbmlModel{nullptr};
  ModelCompartments *modelCompartments{nullptr};
  ModelMembranes *modelMembranes{nullptr};
  const ModelUnits *modelUnits{nullptr};
  Settings *sbmlAnnotation{nullptr};
  bool hasUnsavedChanges{false};
  int importDimensions(const libsbml::Model *model);
  void convertSBMLGeometryTo3d();
  void writeDefaultGeometryToSBML();
  void updateCompartmentAndMembraneSizes();

public:
  /**
   * @brief Number of spatial dimensions in analytic geometry file.
   * @param filename Analytic-geometry file.
   * @returns Number of dimensions if file contains analytic geometry.
   */
  [[nodiscard]] static std::optional<int>
  getAnalyticGeometryNumDimensions(const QString &filename);
  /**
   * @brief Default raster size for analytic geometry import.
   * @param filename Analytic-geometry file.
   * @returns Suggested image size if available.
   */
  [[nodiscard]] static std::optional<common::Volume>
  getDefaultAnalyticGeometryImageSize(const QString &filename);
  /**
   * @brief Preview rasterized analytic geometry.
   * @param filename Analytic-geometry file.
   * @param imageSize Requested preview image size.
   * @returns Preview image stack.
   */
  [[nodiscard]] static common::ImageStack
  getAnalyticGeometryPreview(const QString &filename,
                             const common::Volume &imageSize);
  /**
   * @brief Construct empty geometry model.
   */
  ModelGeometry();
  /**
   * @brief Construct geometry model from SBML model and dependencies.
   * @param model SBML model pointer.
   * @param compartments Compartment manager.
   * @param membranes Membrane manager.
   * @param units Units manager.
   * @param annotation Settings annotation.
   */
  explicit ModelGeometry(libsbml::Model *model, ModelCompartments *compartments,
                         ModelMembranes *membranes, const ModelUnits *units,
                         Settings *annotation);
  /**
   * @brief Import sampled-field geometry from SBML model.
   * @param model Source SBML model.
   * @param analyticImageSize Optional raster size for analytic fallback.
   */
  void importSampledFieldGeometry(
      const libsbml::Model *model,
      const std::optional<common::Volume> &analyticImageSize = std::nullopt);
  /**
   * @brief Import sampled-field/analytic geometry from file.
   * @param filename Source filename.
   * @param analyticImageSize Optional raster size for analytic fallback.
   */
  void importSampledFieldGeometry(
      const QString &filename,
      const std::optional<common::Volume> &analyticImageSize = std::nullopt);
  /**
   * @brief Replace geometry from image stack.
   * @param imgs Geometry images.
   * @param keepColorAssignments Keep previous compartment-color mapping.
   */
  void importGeometryFromImages(const common::ImageStack &imgs,
                                bool keepColorAssignments);
  /**
   * @brief Rebuild mesh objects from current geometry.
   */
  void updateMesh();
  /**
   * @brief Clear geometry and mesh state.
   */
  void clear();
  /**
   * @brief Number of spatial dimensions.
   * @returns Number of spatial dimensions.
   */
  [[nodiscard]] int getNumDimensions() const;
  /**
   * @brief Physical voxel size.
   * @returns Physical voxel size.
   */
  [[nodiscard]] const common::VolumeF &getVoxelSize() const;
  /**
   * @brief Set physical voxel size and optionally write to SBML.
   * @param newVoxelSize New voxel size.
   * @param updateSBML If ``true``, write updates back to SBML.
   */
  void setVoxelSize(const common::VolumeF &newVoxelSize,
                    bool updateSBML = true);
  /**
   * @brief Physical origin of voxel ``(0,0,0)``.
   * @returns Physical origin.
   */
  [[nodiscard]] const common::VoxelF &getPhysicalOrigin() const;
  /**
   * @brief Physical total geometry size.
   * @returns Physical geometry size.
   */
  [[nodiscard]] const common::VolumeF &getPhysicalSize() const;
  /**
   * @brief Convert voxel index to physical coordinate.
   * @param voxel Voxel index.
   * @returns Physical coordinate of voxel center.
   */
  [[nodiscard]] common::VoxelF
  getPhysicalPoint(const common::Voxel &voxel) const;
  /**
   * @brief Convert voxel index to formatted physical coordinate string.
   * @param voxel Voxel index.
   * @returns Formatted coordinate string.
   */
  [[nodiscard]] QString
  getPhysicalPointAsString(const common::Voxel &voxel) const;
  /**
   * @brief Geometry images.
   * @returns Geometry image stack.
   */
  [[nodiscard]] const common::ImageStack &getImages() const;
  /**
   * @brief 2D mesh object (nullable).
   * @returns 2D mesh pointer or ``nullptr``.
   */
  [[nodiscard]] mesh::Mesh2d *getMesh2d() const;
  /**
   * @brief 3D mesh object (nullable).
   * @returns 3D mesh pointer or ``nullptr``.
   */
  [[nodiscard]] mesh::Mesh3d *getMesh3d() const;
  /**
   * @brief Returns ``true`` if geometry is valid.
   * @returns Geometry validity.
   */
  [[nodiscard]] bool getIsValid() const;
  /**
   * @brief Returns ``true`` if mesh is valid.
   * @returns Mesh validity.
   */
  [[nodiscard]] bool getIsMeshValid() const;
  /**
   * @brief Returns ``true`` if geometry image data is available.
   * @returns ``true`` if geometry images exist.
   */
  [[nodiscard]] bool getHasImage() const;
  /**
   * @brief Replace one geometry color with another.
   * @param oldColor Existing color.
   * @param newColor Replacement color.
   */
  void updateGeometryImageColor(QRgb oldColor, QRgb newColor);
  /**
   * @brief Write current geometry representation to SBML.
   */
  void writeGeometryToSBML() const;
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

} // namespace model

} // namespace sme
