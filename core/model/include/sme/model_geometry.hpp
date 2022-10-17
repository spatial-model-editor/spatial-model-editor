// SBML Model Geometry
//   - import geometry from SBML sampled field geometry

#pragma once

#include "sme/geometry.hpp"
#include <QImage>
#include <QRgb>
#include <memory>
#include <string>
#include <vector>

namespace libsbml {
class Model;
}

namespace sme {

namespace mesh {
class Mesh;
}

namespace model {

class ModelCompartments;
class ModelMembranes;
class ModelUnits;
struct Settings;

class ModelGeometry {
private:
  geometry::VoxelF physicalOrigin{};
  geometry::VSizeF physicalSize{};
  geometry::VSizeF voxelSize{1.0, 1.0, 1.0};
  int numDimensions{3};
  std::vector<QImage> images;
  std::unique_ptr<mesh::Mesh> mesh;
  bool isValid{false};
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
  ModelGeometry();
  explicit ModelGeometry(libsbml::Model *model, ModelCompartments *compartments,
                         ModelMembranes *membranes, const ModelUnits *units,
                         Settings *annotation);
  void importSampledFieldGeometry(const libsbml::Model *model);
  void importSampledFieldGeometry(const QString &filename);
  void importGeometryFromImages(const std::vector<QImage> &imgs,
                                bool keepColourAssignments);
  void updateMesh();
  void clear();
  [[nodiscard]] int getNumDimensions() const;
  [[nodiscard]] const geometry::VSizeF &getVoxelSize() const;
  void setVoxelSize(const geometry::VSizeF &newVoxelSize,
                    bool updateSBML = true);
  [[nodiscard]] const geometry::VoxelF &getPhysicalOrigin() const;
  [[nodiscard]] const geometry::VSizeF &getPhysicalSize() const;
  [[nodiscard]] geometry::VoxelF
  getPhysicalPoint(const geometry::Voxel &voxel) const;
  [[nodiscard]] QString
  getPhysicalPointAsString(const geometry::Voxel &voxel) const;
  [[nodiscard]] const std::vector<QImage> &getImages() const;
  [[nodiscard]] mesh::Mesh *getMesh() const;
  [[nodiscard]] bool getIsValid() const;
  [[nodiscard]] bool getHasImage() const;
  void writeGeometryToSBML() const;
  [[nodiscard]] bool getHasUnsavedChanges() const;
  void setHasUnsavedChanges(bool unsavedChanges);
};

} // namespace model

} // namespace sme
