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
  [[nodiscard]] static std::optional<int>
  getAnalyticGeometryNumDimensions(const QString &filename);
  [[nodiscard]] static std::optional<common::Volume>
  getDefaultAnalyticGeometryImageSize(const QString &filename);
  [[nodiscard]] static common::ImageStack
  getAnalyticGeometryPreview(const QString &filename,
                             const common::Volume &imageSize);
  ModelGeometry();
  explicit ModelGeometry(libsbml::Model *model, ModelCompartments *compartments,
                         ModelMembranes *membranes, const ModelUnits *units,
                         Settings *annotation);
  void importSampledFieldGeometry(
      const libsbml::Model *model,
      const std::optional<common::Volume> &analyticImageSize = std::nullopt);
  void importSampledFieldGeometry(
      const QString &filename,
      const std::optional<common::Volume> &analyticImageSize = std::nullopt);
  void importGeometryFromImages(const common::ImageStack &imgs,
                                bool keepColorAssignments);
  void updateMesh();
  void clear();
  [[nodiscard]] int getNumDimensions() const;
  [[nodiscard]] const common::VolumeF &getVoxelSize() const;
  void setVoxelSize(const common::VolumeF &newVoxelSize,
                    bool updateSBML = true);
  [[nodiscard]] const common::VoxelF &getPhysicalOrigin() const;
  [[nodiscard]] const common::VolumeF &getPhysicalSize() const;
  [[nodiscard]] common::VoxelF
  getPhysicalPoint(const common::Voxel &voxel) const;
  [[nodiscard]] QString
  getPhysicalPointAsString(const common::Voxel &voxel) const;
  [[nodiscard]] const common::ImageStack &getImages() const;
  [[nodiscard]] mesh::Mesh2d *getMesh2d() const;
  [[nodiscard]] mesh::Mesh3d *getMesh3d() const;
  [[nodiscard]] bool getIsValid() const;
  [[nodiscard]] bool getIsMeshValid() const;
  [[nodiscard]] bool getHasImage() const;
  void updateGeometryImageColor(QRgb oldColor, QRgb newColor);
  void writeGeometryToSBML() const;
  [[nodiscard]] bool getHasUnsavedChanges() const;
  void setHasUnsavedChanges(bool unsavedChanges);
};

} // namespace model

} // namespace sme
