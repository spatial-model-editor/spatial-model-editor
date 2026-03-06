// SBML Model Geometry
//   - import geometry from SBML sampled field geometry

#pragma once

#include "sme/geometry.hpp"
#include "sme/gmsh.hpp"
#include "sme/image_stack.hpp"
#include "sme/mesh2d.hpp"
#include "sme/mesh_types.hpp"
#include <QImage>
#include <QRgb>
#include <memory>
#include <optional>
#include <string>
#include <utility>
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
  struct TaggedFixedTopology2d {
    mesh::GMSHMesh mesh{};
    std::vector<std::pair<QRgb, int>> colorTagPairs{};
  };
  common::VoxelF physicalOrigin{};
  common::VolumeF physicalSize{};
  common::VolumeF voxelSize{1.0, 1.0, 1.0};
  int numDimensions{3};
  common::ImageStack images;
  std::unique_ptr<mesh::Mesh2d> mesh;
  std::unique_ptr<mesh::Mesh3d> mesh3d;
  std::optional<mesh::FixedTopology3d> importedMesh3d;
  std::optional<mesh::FixedTopology2d> importedMesh2d;
  std::optional<TaggedFixedTopology2d> importedTaggedMesh2d;
  QString fixedMeshImportDiagnostic;
  mutable QString fixedMeshExportDiagnostic;
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
   * @brief Import geometry rasterized from a Gmsh mesh and align physical
   * coordinates with the mesh bounds.
   * @param imgs Voxelized geometry image stack.
   * @param gmshMesh Source Gmsh mesh used for voxelization.
   * @param importAsFixedTopology Store mesh as fixed topology if ``true``.
   */
  void importGeometryFromGmsh(const common::ImageStack &imgs,
                              const mesh::GMSHMesh &gmshMesh,
                              bool importAsFixedTopology);
  /**
   * @brief Store imported Gmsh mesh for fixed-topology meshing.
   * @param gmshMesh Imported Gmsh mesh (tetrahedra for 3D or triangles for 2D).
   * @param colorTagPairs Optional explicit mapping from compartment color to
   * Gmsh physical tag.
   */
  void setImportedGmshMesh(
      const mesh::GMSHMesh &gmshMesh,
      const std::optional<std::vector<std::pair<QRgb, int>>> &colorTagPairs =
          std::nullopt);
  /**
   * @brief Store imported fixed 2d mesh topology for fixed-topology meshing.
   * @param fixedMesh2d Imported 2d fixed mesh.
   */
  void setImportedMesh2d(const mesh::FixedTopology2d &fixedMesh2d);
  /**
   * @brief Capture current active mesh as fixed-topology mesh.
   *
   * Captures current 2D or 3D mesh (if valid) and stores it as the fixed mesh
   * topology source for subsequent remeshing.
   */
  void captureCurrentMeshAsFixedTopology();
  /**
   * @brief Clear any stored imported mesh topology.
   */
  void clearImportedMesh();
  /**
   * @brief Returns ``true`` if imported mesh topology is available.
   * @returns ``true`` if imported mesh exists.
   */
  [[nodiscard]] bool hasImportedMesh() const;
  /**
   * @brief Diagnostic message from ParametricGeometry fixed-mesh import.
   * @returns Empty string if no import diagnostic is available.
   */
  [[nodiscard]] const QString &getFixedMeshImportDiagnostic() const;
  /**
   * @brief Diagnostic message from ParametricGeometry fixed-mesh export.
   * @returns Empty string if no export diagnostic is available.
   */
  [[nodiscard]] const QString &getFixedMeshExportDiagnostic() const;
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
   * @brief Set physical origin and optionally write to SBML.
   * @param newPhysicalOrigin New physical origin.
   * @param updateSBML If ``true``, write updates back to SBML.
   */
  void setPhysicalOrigin(const common::VoxelF &newPhysicalOrigin,
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
