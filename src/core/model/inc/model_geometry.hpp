// SBML Model Geometry
//   - import geometry from SBML sampled field geometry

#pragma once

#include <QImage>
#include <QRgb>
#include <memory>
#include <string>
#include <vector>

#include "geometry.hpp"

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
  double pixelWidth{1.0};
  double pixelDepth{1.0};
  QPointF physicalOrigin{QPointF(0, 0)};
  double zOrigin{0.0};
  QSizeF physicalSize{QSizeF(0, 0)};
  int numDimensions{3};
  QImage image;
  std::unique_ptr<mesh::Mesh> mesh;
  bool isValid{false};
  bool hasImage{false};
  libsbml::Model *sbmlModel{nullptr};
  ModelCompartments *modelCompartments{nullptr};
  ModelMembranes *modelMembranes{nullptr};
  const ModelUnits *modelUnits{nullptr};
  Settings *sbmlAnnotation = nullptr;
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
  void importParametricGeometry(const libsbml::Model *model,
                                const Settings *settings);
  void importSampledFieldGeometry(const QString &filename);
  void importGeometryFromImage(const QImage &img, bool keepColourAssignments);
  void updateMesh();
  void clear();
  [[nodiscard]] int getNumDimensions() const;
  [[nodiscard]] double getPixelWidth() const;
  void setPixelWidth(double width, bool updateSBML = true);
  [[nodiscard]] double getPixelDepth() const;
  void setPixelDepth(double depth);
  [[nodiscard]] double getZOrigin() const;
  [[nodiscard]] const QPointF &getPhysicalOrigin() const;
  [[nodiscard]] const QSizeF &getPhysicalSize() const;
  [[nodiscard]] QPointF getPhysicalPoint(const QPoint pixel) const;
  [[nodiscard]] QString getPhysicalPointAsString(const QPoint pixel) const;
  [[nodiscard]] const QImage &getImage() const;
  [[nodiscard]] mesh::Mesh *getMesh() const;
  [[nodiscard]] bool getIsValid() const;
  [[nodiscard]] bool getHasImage() const;
  void writeGeometryToSBML() const;
  [[nodiscard]] bool getHasUnsavedChanges() const;
  void setHasUnsavedChanges(bool unsavedChanges);
};

} // namespace model

} // namespace sme
