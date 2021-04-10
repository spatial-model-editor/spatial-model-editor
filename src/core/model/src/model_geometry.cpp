#include "model_geometry.hpp"
#include "geometry_analytic.hpp"
#include "geometry_parametric.hpp"
#include "geometry_sampled_field.hpp"
#include "logger.hpp"
#include "mesh.hpp"
#include "model_compartments.hpp"
#include "model_membranes.hpp"
#include "model_units.hpp"
#include "sbml_utils.hpp"
#include "utils.hpp"
#include "xml_annotation.hpp"
#include <memory>
#include <sbml/SBMLTypes.h>
#include <sbml/extension/SBMLDocumentPlugin.h>
#include <sbml/packages/spatial/common/SpatialExtensionTypes.h>
#include <sbml/packages/spatial/extension/SpatialExtension.h>

namespace sme::model {

int ModelGeometry::importDimensions(const libsbml::Model *model) {
  const auto *geom{getGeometry(model)};
  if (geom == nullptr) {
    return 0;
  }
  auto nDim{geom->getNumCoordinateComponents()};
  SPDLOG_INFO("Importing existing {}d SBML model geometry", nDim);
  const auto *xcoord{geom->getCoordinateComponentByKind(
      libsbml::CoordinateKind_t::SPATIAL_COORDINATEKIND_CARTESIAN_X)};
  if (xcoord == nullptr) {
    SPDLOG_ERROR("No x-coordinate found in SBML model");
    return 0;
  }
  const auto *ycoord{geom->getCoordinateComponentByKind(
      libsbml::CoordinateKind_t::SPATIAL_COORDINATEKIND_CARTESIAN_Y)};
  if (ycoord == nullptr) {
    SPDLOG_ERROR("No y-coordinate found in SBML model");
    return 0;
  }

  // import xy coordinates
  double xmin{xcoord->getBoundaryMin()->getValue()};
  double xmax{xcoord->getBoundaryMax()->getValue()};
  double ymin{ycoord->getBoundaryMin()->getValue()};
  double ymax{ycoord->getBoundaryMax()->getValue()};
  SPDLOG_INFO("  - found x range [{},{}]", xmin, xmax);
  SPDLOG_INFO("  - found y range [{},{}]", ymin, ymax);
  physicalOrigin = QPointF(xmin, ymin);
  SPDLOG_INFO("  -> origin [{},{}]", physicalOrigin.x(), physicalOrigin.y());
  physicalSize = QSizeF(xmax - xmin, ymax - ymin);
  SPDLOG_INFO("  -> size [{},{}]", physicalSize.width(), physicalSize.height());
  if (nDim == 3) {
    const auto *zcoord{geom->getCoordinateComponentByKind(
        libsbml::CoordinateKind_t::SPATIAL_COORDINATEKIND_CARTESIAN_Z)};
    if (zcoord == nullptr) {
      SPDLOG_ERROR("No z-coordinate found in 3d SBML model");
      return 0;
    }
    zOrigin = zcoord->getBoundaryMin()->getValue();
    SPDLOG_INFO("  - found z origin {}", zOrigin);
    pixelDepth = zcoord->getBoundaryMax()->getValue() - zOrigin;
    SPDLOG_INFO("  - found z depth / pixel depth {}", pixelDepth);
  }
  return static_cast<int>(nDim);
}

static void createZCoordinateComponent(libsbml::Model *model) {
  auto *geom{getOrCreateGeometry(model)};
  auto *coordZ{geom->getCoordinateComponent(2)};
  if (coordZ == nullptr) {
    coordZ = geom->createCoordinateComponent();
  }
  coordZ->setType(
      libsbml::CoordinateKind_t::SPATIAL_COORDINATEKIND_CARTESIAN_Z);
  coordZ->setId("zCoord");
  auto *paramZ = model->createParameter();
  paramZ->setId("z");
  paramZ->setUnits(model->getLengthUnits());
  paramZ->setConstant(true);
  paramZ->setValue(0);
  auto *ssr{static_cast<libsbml::SpatialParameterPlugin *>(
                paramZ->getPlugin("spatial"))
                ->createSpatialSymbolReference()};
  ssr->setSpatialRef(coordZ->getId());
  SPDLOG_INFO("  - creating Parameter: {}", paramZ->getId());
  SPDLOG_INFO("  - with spatialSymbolReference: {}", ssr->getSpatialRef());
  auto *minZ = coordZ->createBoundaryMin();
  minZ->setId("zBoundaryMin");
  minZ->setValue(0.0);
  auto *maxZ = coordZ->createBoundaryMax();
  maxZ->setId("zBoundaryMax");
  maxZ->setValue(1.0);
  SPDLOG_INFO("  - z in range [{},{}]", minZ->getValue(), maxZ->getValue());
}

void ModelGeometry::convertSBMLGeometryTo3d() {
  SPDLOG_INFO("Converting 2d SBML model geometry to 3d");
  createZCoordinateComponent(sbmlModel);
  for (unsigned i = 0; i < sbmlModel->getNumCompartments(); ++i) {
    auto *comp{sbmlModel->getCompartment(i)};
    if (comp == nullptr) {
      return;
    }
    auto nDim{comp->getSpatialDimensions() + 1};
    SPDLOG_INFO("Setting compartment '{}' dimensions to {}", comp->getId(),
                nDim);
    comp->setSpatialDimensions(nDim);
  }
}

void ModelGeometry::writeDefaultGeometryToSBML() {
  SPDLOG_INFO("Creating new 3d SBML model geometry");
  numDimensions = 3;
  auto *spatial = static_cast<libsbml::SpatialModelPlugin *>(
      sbmlModel->getPlugin("spatial"));
  auto *geom = spatial->createGeometry();
  geom->setCoordinateSystem(
      libsbml::GeometryKind_t::SPATIAL_GEOMETRYKIND_CARTESIAN);
  for (int i = 0; i < numDimensions; ++i) {
    geom->createCoordinateComponent();
  }
  for (unsigned i = 0; i < sbmlModel->getNumCompartments(); ++i) {
    auto *comp = sbmlModel->getCompartment(i);
    comp->setSpatialDimensions(static_cast<unsigned int>(numDimensions));
  }

  // set-up xyz coordinates
  auto *coordX = geom->getCoordinateComponent(0);
  coordX->setType(
      libsbml::CoordinateKind_t::SPATIAL_COORDINATEKIND_CARTESIAN_X);
  coordX->setId("xCoord");
  auto *paramX = sbmlModel->createParameter();
  paramX->setId("x");
  paramX->setUnits(sbmlModel->getLengthUnits());
  paramX->setConstant(true);
  paramX->setValue(0);
  auto *ssr = static_cast<libsbml::SpatialParameterPlugin *>(
                  paramX->getPlugin("spatial"))
                  ->createSpatialSymbolReference();
  ssr->setSpatialRef(coordX->getId());
  SPDLOG_INFO("  - creating Parameter: {}", paramX->getId());
  SPDLOG_INFO("  - with spatialSymbolReference: {}", ssr->getSpatialRef());
  auto *minX = coordX->createBoundaryMin();
  minX->setId("xBoundaryMin");
  minX->setValue(0);
  auto *maxX = coordX->createBoundaryMax();
  maxX->setId("xBoundaryMax");
  maxX->setValue(pixelWidth * static_cast<double>(image.width()));
  SPDLOG_INFO("  - x in range [{},{}]", minX->getValue(), maxX->getValue());

  auto *coordY = geom->getCoordinateComponent(1);
  coordY->setType(
      libsbml::CoordinateKind_t::SPATIAL_COORDINATEKIND_CARTESIAN_Y);
  coordY->setId("yCoord");
  auto *paramY = sbmlModel->createParameter();
  paramY->setId("y");
  paramY->setUnits(sbmlModel->getLengthUnits());
  paramY->setConstant(true);
  paramY->setValue(0);
  ssr = static_cast<libsbml::SpatialParameterPlugin *>(
            paramY->getPlugin("spatial"))
            ->createSpatialSymbolReference();
  ssr->setSpatialRef(coordY->getId());
  SPDLOG_INFO("  - creating Parameter: {}", ssr->getId());
  SPDLOG_INFO("  - with spatialSymbolReference: {}", ssr->getSpatialRef());
  auto *minY = coordY->createBoundaryMin();
  minY->setId("yBoundaryMin");
  minY->setValue(0);
  auto *maxY = coordY->createBoundaryMax();
  maxY->setId("yBoundaryMax");
  maxY->setValue(pixelWidth * static_cast<double>(image.height()));
  SPDLOG_INFO("  - y in range [{},{}]", minY->getValue(), maxY->getValue());
  createZCoordinateComponent(sbmlModel);
  // the above overwrote any existing compartment geometry, so re-create
  // defaults here
  createDefaultCompartmentGeometryIfMissing(sbmlModel);
}

void ModelGeometry::updateCompartmentAndMembraneSizes() {
  // reassign all compartment colours to update sizes, interior points, etc
  for (const auto &compartmentId : modelCompartments->getIds()) {
    modelCompartments->setColour(compartmentId,
                                 modelCompartments->getColour(compartmentId));
  }
  modelMembranes->exportToSBML(pixelWidth * pixelDepth);
}

ModelGeometry::ModelGeometry() = default;

ModelGeometry::ModelGeometry(libsbml::Model *model,
                             ModelCompartments *compartments,
                             ModelMembranes *membranes, const ModelUnits *units,
                             Settings *annotation)
    : sbmlModel{model}, modelCompartments{compartments},
      modelMembranes{membranes}, modelUnits{units}, sbmlAnnotation{annotation} {
  if (auto nDim{importDimensions(model)}; nDim == 0) {
    SPDLOG_WARN("Failed to import geometry");
    writeDefaultGeometryToSBML();
    return;
  } else if (nDim == 2) {
    convertSBMLGeometryTo3d();
    return;
  }
}

static double calculatePixelWidth(const QSize &imageSize,
                                  const QSizeF &physicalSize) {
  // calculate pixel size from image dimensions
  double xPixels = static_cast<double>(imageSize.width());
  double xPixelSize = physicalSize.width() / xPixels;
  double yPixels = static_cast<double>(imageSize.height());
  double yPixelSize = physicalSize.height() / yPixels;
  constexpr double maxRelativeDifference{1e-12};
  if (std::abs((xPixelSize - yPixelSize) / xPixelSize) >
      maxRelativeDifference) {
    SPDLOG_WARN("Pixels are not square: {} x {}", xPixelSize, yPixelSize);
  }
  return xPixelSize;
}

void ModelGeometry::importSampledFieldGeometry(const libsbml::Model *model) {
  importDimensions(model);
  auto gsf = importGeometryFromSampledField(getGeometry(model));
  if (gsf.image.isNull()) {
    SPDLOG_INFO(
        "No Sampled Field Geometry found - looking for Analytic Geometry...");
    gsf =
        importGeometryFromAnalyticGeometry(model, physicalOrigin, physicalSize);
    if (gsf.image.isNull()) {
      SPDLOG_INFO("No Analytic Geometry found");
      return;
    }
  }
  hasUnsavedChanges = true;
  SPDLOG_INFO("  - found {}x{} geometry image", gsf.image.width(),
              gsf.image.height());
  image = gsf.image.convertToFormat(QImage::Format_Indexed8);
  hasImage = true;
  pixelWidth = calculatePixelWidth(image.size(), physicalSize);
  setPixelWidth(pixelWidth, false);
  for (const auto &[id, colour] : gsf.compartmentIdColourPairs) {
    SPDLOG_INFO("setting compartment {} colour to {:x}", id, colour);
    modelCompartments->setColour(id.c_str(), colour);
  }
  auto *geom = getOrCreateGeometry(sbmlModel);
  exportSampledFieldGeometry(geom, image);
}

void ModelGeometry::importSampledFieldGeometry(const QString &filename) {
  std::unique_ptr<libsbml::SBMLDocument> doc{
      libsbml::readSBMLFromFile(filename.toStdString().c_str())};
  importSampledFieldGeometry(doc->getModel());
}

void ModelGeometry::importGeometryFromImage(const QImage &img,
                                            bool keepColourAssignments) {
  hasUnsavedChanges = true;
  const auto &ids{modelCompartments->getIds()};
  auto oldColours{modelCompartments->getColours()};
  for (const auto &id : ids) {
    modelCompartments->setColour(id, 0);
  }
  constexpr auto flagNoDither{Qt::AvoidDither | Qt::ThresholdDither |
                              Qt::ThresholdAlphaDither | Qt::NoOpaqueDetection};
  auto imgNoAlpha{img};
  if (img.hasAlphaChannel()) {
    SPDLOG_WARN("ignoring alpha channel");
    imgNoAlpha = img.convertToFormat(QImage::Format_RGB32, flagNoDither);
  }
  image = imgNoAlpha.convertToFormat(QImage::Format_Indexed8, flagNoDither);
  auto *geom = getOrCreateGeometry(sbmlModel);
  exportSampledFieldGeometry(geom, image);
  if (keepColourAssignments) {
    for (int i = 0; i < ids.size(); ++i) {
      modelCompartments->setColour(ids[i], oldColours[i]);
    }
  }
  hasImage = true;
}

void ModelGeometry::updateMesh() {
  // geometry only valid if all compartments have a colour
  isValid = hasImage && !modelCompartments->getColours().empty() &&
            !modelCompartments->getColours().contains(0);
  if (!isValid) {
    mesh.reset();
    return;
  }
  hasUnsavedChanges = true;
  const auto &colours{modelCompartments->getColours()};
  const auto &ids{modelCompartments->getIds()};
  const auto &meshParams{sbmlAnnotation->meshParameters};
  mesh = std::make_unique<mesh::Mesh>(
      image, meshParams.maxPoints, meshParams.maxAreas, pixelWidth,
      physicalOrigin, common::toStdVec(colours));
  for (int i = 0; i < ids.size(); ++i) {
    modelCompartments->setInteriorPoints(
        ids[i],
        mesh->getCompartmentInteriorPoints()[static_cast<std::size_t>(i)]);
  }
  modelMembranes->updateCompartmentImage(image, mesh->getPixelCornersImage());
}

void ModelGeometry::clear() {
  mesh.reset();
  hasImage = false;
  isValid = false;
  image = {};
  auto *model = sbmlModel;
  hasUnsavedChanges = true;
  if (model == nullptr) {
    return;
  }
  // remove any SBML geometry-related stuff
  sbmlAnnotation->meshParameters = {};
  for (unsigned i = 0; i < model->getNumCompartments(); ++i) {
    auto *comp = model->getCompartment(i);
    auto *scp = static_cast<libsbml::SpatialCompartmentPlugin *>(
        comp->getPlugin("spatial"));
    if (scp != nullptr && scp->isSetCompartmentMapping()) {
      scp->unsetCompartmentMapping();
    }
  }
  auto *spatial = static_cast<libsbml::SpatialModelPlugin *>(
      sbmlModel->getPlugin("spatial"));
  if (spatial != nullptr && spatial->isSetGeometry()) {
    auto *g = spatial->getGeometry();
    for (unsigned i = 0; i < g->getNumGeometryDefinitions(); ++i) {
      std::unique_ptr<libsbml::GeometryDefinition> gd(
          g->removeGeometryDefinition(i));
      SPDLOG_INFO("removing GeometryDefinition {}", gd->getId());
    }
    for (unsigned i = 0; i < g->getNumDomainTypes(); ++i) {
      std::unique_ptr<libsbml::DomainType> dt(g->removeDomainType(i));
      SPDLOG_INFO("removing DomainType {}", dt->getId());
    }
    for (unsigned i = 0; i < g->getNumDomains(); ++i) {
      std::unique_ptr<libsbml::Domain> d(g->removeDomain(i));
      SPDLOG_INFO("removing Domain {}", d->getId());
    }
    for (unsigned i = 0; i < g->getNumSampledFields(); ++i) {
      std::unique_ptr<libsbml::SampledField> sf(g->removeSampledField(i));
      SPDLOG_INFO("removing SampledField {}", sf->getId());
    }
  }
}

int ModelGeometry::getNumDimensions() const { return numDimensions; }

double ModelGeometry::getPixelWidth() const { return pixelWidth; }

void ModelGeometry::setPixelWidth(double width, bool updateSBML) {
  SPDLOG_INFO("Setting pixel width to {}", width);
  hasUnsavedChanges = true;
  double oldWidth = pixelWidth;
  pixelWidth = width;
  SPDLOG_INFO("New pixel width = {}", pixelWidth);

  // update origin
  physicalOrigin *= width / oldWidth;
  SPDLOG_INFO("  - origin rescaled to ({},{})", physicalOrigin.x(),
              physicalOrigin.y());

  if (mesh != nullptr) {
    mesh->setPhysicalGeometry(width, physicalOrigin);
  }
  // update xy coordinates
  auto *geom = getOrCreateGeometry(sbmlModel);
  physicalSize = {pixelWidth * static_cast<double>(image.width()),
                  pixelWidth * static_cast<double>(image.height())};
  auto *coord = geom->getCoordinateComponentByKind(
      libsbml::CoordinateKind_t::SPATIAL_COORDINATEKIND_CARTESIAN_X);
  auto *min = coord->getBoundaryMin();
  auto *max = coord->getBoundaryMax();
  min->setValue(physicalOrigin.x());
  max->setValue(physicalOrigin.x() + physicalSize.width());
  SPDLOG_INFO("  - x now in range [{},{}]", min->getValue(), max->getValue());
  coord = geom->getCoordinateComponentByKind(
      libsbml::CoordinateKind_t::SPATIAL_COORDINATEKIND_CARTESIAN_Y);
  min = coord->getBoundaryMin();
  max = coord->getBoundaryMax();
  min->setValue(physicalOrigin.y());
  max->setValue(physicalOrigin.y() + physicalSize.height());
  SPDLOG_INFO("  - y now in range [{},{}]", min->getValue(), max->getValue());
  if (updateSBML) {
    updateCompartmentAndMembraneSizes();
  }
}

double ModelGeometry::getPixelDepth() const { return pixelDepth; }

void ModelGeometry::setPixelDepth(double depth) {
  SPDLOG_INFO("Setting pixel depth to {}", depth);
  pixelDepth = depth;
  auto *geom{getOrCreateGeometry(sbmlModel)};
  auto *coord{geom->getCoordinateComponentByKind(
      libsbml::CoordinateKind_t::SPATIAL_COORDINATEKIND_CARTESIAN_Z)};
  auto *min{coord->getBoundaryMin()};
  auto *max{coord->getBoundaryMax()};
  min->setValue(zOrigin);
  max->setValue(zOrigin + pixelDepth);
  SPDLOG_INFO("  - z now in range [{},{}]", min->getValue(), max->getValue());
  updateCompartmentAndMembraneSizes();
}

double ModelGeometry::getZOrigin() const { return zOrigin; }

const QPointF &ModelGeometry::getPhysicalOrigin() const {
  return physicalOrigin;
}

const QSizeF &ModelGeometry::getPhysicalSize() const { return physicalSize; }

QPointF ModelGeometry::getPhysicalPoint(const QPoint pixel) const {
  QPointF physicalPoint{physicalOrigin};
  physicalPoint.rx() += pixelWidth * static_cast<double>(pixel.x());
  physicalPoint.ry() +=
      pixelWidth * static_cast<double>(image.height() - 1 - pixel.y());
  return physicalPoint;
}

QString ModelGeometry::getPhysicalPointAsString(const QPoint pixel) const {
  auto lengthUnit{modelUnits->getLength().name};
  auto physicalPoint{getPhysicalPoint(pixel)};
  return QString("x: %1 %2, y: %3 %2")
      .arg(physicalPoint.x())
      .arg(lengthUnit)
      .arg(physicalPoint.y());
}

const QImage &ModelGeometry::getImage() const { return image; }

mesh::Mesh *ModelGeometry::getMesh() const { return mesh.get(); }

bool ModelGeometry::getIsValid() const { return isValid; }

bool ModelGeometry::getHasImage() const { return hasImage; }

void ModelGeometry::writeGeometryToSBML() const {
  if (mesh != nullptr && mesh->isValid()) {
    sbmlAnnotation->meshParameters = {mesh->getBoundaryMaxPoints(),
                                      mesh->getCompartmentMaxTriangleArea()};
  } else {
    sbmlAnnotation->meshParameters = {};
  }
  writeGeometryMeshToSBML(sbmlModel, mesh.get(), *modelCompartments);
}

bool ModelGeometry::getHasUnsavedChanges() const { return hasUnsavedChanges; }

void ModelGeometry::setHasUnsavedChanges(bool unsavedChanges) {
  hasUnsavedChanges = unsavedChanges;
}

} // namespace sme::model
