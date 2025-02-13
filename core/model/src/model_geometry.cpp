#include "sme/model_geometry.hpp"
#include "geometry_analytic.hpp"
#include "geometry_sampled_field.hpp"
#include "sbml_utils.hpp"
#include "sme/logger.hpp"
#include "sme/mesh2d.hpp"
#include "sme/mesh3d.hpp"
#include "sme/model_compartments.hpp"
#include "sme/model_membranes.hpp"
#include "sme/model_units.hpp"
#include "sme/utils.hpp"
#include "sme/xml_annotation.hpp"
#include <memory>
#include <sbml/SBMLTypes.h>
#include <sbml/extension/SBMLDocumentPlugin.h>
#include <sbml/packages/spatial/common/SpatialExtensionTypes.h>
#include <sbml/packages/spatial/extension/SpatialExtension.h>
#include <stdexcept>

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
  // default z coordinates
  double zmin{0.0};
  double zmax{1.0};
  if (nDim == 3) {
    const auto *zcoord{geom->getCoordinateComponentByKind(
        libsbml::CoordinateKind_t::SPATIAL_COORDINATEKIND_CARTESIAN_Z)};
    if (zcoord == nullptr) {
      SPDLOG_ERROR("No z-coordinate found in 3d SBML model");
      return 0;
    }
    zmin = zcoord->getBoundaryMin()->getValue();
    zmax = zcoord->getBoundaryMax()->getValue();
    SPDLOG_INFO("  - found z range [{},{}]", zmin, zmax);
  }
  physicalOrigin = {xmin, ymin, zmin};
  SPDLOG_INFO("  -> origin [{},{},{}]", physicalOrigin.p.x(),
              physicalOrigin.p.y(), physicalOrigin.z);
  physicalSize = {xmax - xmin, ymax - ymin, zmax - zmin};
  SPDLOG_INFO("  -> volume [{},{},{}]", physicalSize.width(),
              physicalSize.height(), physicalSize.depth());
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
  maxX->setValue(voxelSize.width() *
                 static_cast<double>(images.volume().width()));
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
  SPDLOG_INFO("  - creating Parameter: {}", paramY->getId());
  SPDLOG_INFO("  - with spatialSymbolReference: {}", ssr->getSpatialRef());
  auto *minY = coordY->createBoundaryMin();
  minY->setId("yBoundaryMin");
  minY->setValue(0);
  auto *maxY = coordY->createBoundaryMax();
  maxY->setId("yBoundaryMax");
  maxY->setValue(voxelSize.height() *
                 static_cast<double>(images.volume().height()));
  SPDLOG_INFO("  - y in range [{},{}]", minY->getValue(), maxY->getValue());
  createZCoordinateComponent(sbmlModel);
  // the above overwrote any existing compartment geometry, so re-create
  // defaults here
  createDefaultCompartmentGeometryIfMissing(sbmlModel);
}

void ModelGeometry::updateCompartmentAndMembraneSizes() {
  // reassign all compartment colors to update sizes, interior points, etc
  for (const auto &compartmentId : modelCompartments->getIds()) {
    modelCompartments->setColor(compartmentId,
                                modelCompartments->getColor(compartmentId));
  }
  if (isValid) {
    modelMembranes->exportToSBML(voxelSize);
  }
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

static common::VolumeF calculateVoxelSize(const common::Volume &imageSize,
                                          const common::VolumeF &physicalSize) {
  // calculate voxel volume from image dimensions
  double x{physicalSize.width() / static_cast<double>(imageSize.width())};
  double y{physicalSize.height() / static_cast<double>(imageSize.height())};
  double z{physicalSize.depth() / static_cast<double>(imageSize.depth())};
  constexpr double maxRelativeDifference{1e-12};
  if (std::abs((y - x) / x) + std::abs((z - x) / x) > maxRelativeDifference) {
    SPDLOG_WARN("Voxels are not cubes: {} x {} x {}", x, y, z);
  }
  return {x, y, z};
}

void ModelGeometry::importSampledFieldGeometry(const libsbml::Model *model) {
  importDimensions(model);
  auto gsf = importGeometryFromSampledField(getGeometry(model),
                                            sbmlAnnotation->sampledFieldColors);
  if (gsf.images.empty()) {
    SPDLOG_INFO(
        "No Sampled Field Geometry found - looking for Analytic Geometry...");
    gsf =
        importGeometryFromAnalyticGeometry(model, physicalOrigin, physicalSize);
    if (gsf.images.empty()) {
      SPDLOG_INFO("No Analytic Geometry found");
      return;
    }
  }
  hasUnsavedChanges = true;
  SPDLOG_INFO("  - found {}x{}x{} geometry image", gsf.images[0].width(),
              gsf.images[0].height(), gsf.images.size());
  images = common::ImageStack(std::move(gsf.images));
  images.convertToIndexed();
  hasImage = true;
  sbmlAnnotation->sampledFieldColors = common::toStdVec(images.colorTable());
  voxelSize = calculateVoxelSize(images.volume(), physicalSize);
  images.setVoxelSize(voxelSize);
  modelMembranes->updateCompartmentImages(images);
  for (const auto &[id, color] : gsf.compartmentIdColorPairs) {
    SPDLOG_INFO("setting compartment {} color to {:x}", id, color);
    modelCompartments->setColor(id.c_str(), color);
  }
  auto *geom = getOrCreateGeometry(sbmlModel);
  exportSampledFieldGeometry(geom, images);
}

void ModelGeometry::importSampledFieldGeometry(const QString &filename) {
  std::unique_ptr<libsbml::SBMLDocument> doc{
      libsbml::readSBMLFromFile(filename.toStdString().c_str())};
  importSampledFieldGeometry(doc->getModel());
}

void ModelGeometry::importGeometryFromImages(const common::ImageStack &imgs,
                                             bool keepColorAssignments) {
  hasUnsavedChanges = true;
  const auto &ids{modelCompartments->getIds()};
  auto oldColors{modelCompartments->getColors()};
  for (const auto &id : ids) {
    modelCompartments->setColor(id, 0);
  }
  images = common::ImageStack{imgs};
  images.convertToIndexed();
  images.setVoxelSize(voxelSize);
  sbmlAnnotation->sampledFieldColors = common::toStdVec(images.colorTable());
  modelMembranes->updateCompartmentImages(images);
  auto *geom{getOrCreateGeometry(sbmlModel)};
  exportSampledFieldGeometry(geom, images);
  if (keepColorAssignments) {
    for (int i = 0; i < ids.size(); ++i) {
      modelCompartments->setColor(ids[i], oldColors[i]);
    }
  }
  hasImage = true;
}

void ModelGeometry::updateMesh() {
  // geometry only valid if all compartments have a color
  isValid = hasImage && !modelCompartments->getColors().empty() &&
            !modelCompartments->getColors().contains(0);
  if (!isValid) {
    isMeshValid = false;
    mesh.reset();
    mesh3d.reset();
    return;
  }
  hasUnsavedChanges = true;
  const auto &colors{modelCompartments->getColors()};
  const auto &ids{modelCompartments->getIds()};
  const auto &meshParams{sbmlAnnotation->meshParameters};
  if (images.volume().depth() > 1) {
    SPDLOG_INFO("Updating 3d mesh");
    mesh.reset();
    mesh3d = std::make_unique<mesh::Mesh3d>(
        images, std::vector<std::size_t>{}, voxelSize, physicalOrigin,
        common::toStdVec(colors), modelMembranes->getIdColorPairs());
    isMeshValid = mesh3d->isValid();
  } else {
    SPDLOG_INFO("Updating 2d mesh");
    mesh3d.reset();
    mesh = std::make_unique<mesh::Mesh2d>(
        images[0], meshParams.maxPoints, meshParams.maxAreas, voxelSize,
        physicalOrigin, common::toStdVec(colors),
        meshParams.boundarySimplifierType);
    for (int i = 0; i < ids.size(); ++i) {
      modelCompartments->setInteriorPoints(
          ids[i],
          mesh->getCompartmentInteriorPoints()[static_cast<std::size_t>(i)]);
    }
    isMeshValid = mesh->isValid();
  }
}

void ModelGeometry::clear() {
  mesh.reset();
  mesh3d.reset();
  hasImage = false;
  isValid = false;
  isMeshValid = false;
  images.clear();
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

void ModelGeometry::setVoxelSize(const common::VolumeF &newVoxelSize,
                                 bool updateSBML) {
  SPDLOG_INFO("Setting voxel size to {}x{}x{}", newVoxelSize.width(),
              newVoxelSize.height(), newVoxelSize.depth());
  hasUnsavedChanges = true;
  auto oldVoxelSize{voxelSize};
  voxelSize = newVoxelSize;
  images.setVoxelSize(voxelSize);

  // update origin
  physicalOrigin.p.rx() *= voxelSize.width() / oldVoxelSize.width();
  physicalOrigin.p.ry() *= voxelSize.height() / oldVoxelSize.height();
  physicalOrigin.z *= voxelSize.depth() / oldVoxelSize.depth();
  SPDLOG_INFO("  - origin rescaled to ({},{},{})", physicalOrigin.p.x(),
              physicalOrigin.p.y(), physicalOrigin.z);

  if (mesh != nullptr) {
    mesh->setPhysicalGeometry(voxelSize, physicalOrigin);
  }
  // update xyz coordinates
  auto *geom = getOrCreateGeometry(sbmlModel);
  physicalSize = voxelSize * images.volume();
  auto *coord = geom->getCoordinateComponentByKind(
      libsbml::CoordinateKind_t::SPATIAL_COORDINATEKIND_CARTESIAN_X);
  auto *min = coord->getBoundaryMin();
  auto *max = coord->getBoundaryMax();
  min->setValue(physicalOrigin.p.x());
  max->setValue(physicalOrigin.p.x() + physicalSize.width());
  SPDLOG_INFO("  - x now in range [{},{}]", min->getValue(), max->getValue());
  coord = geom->getCoordinateComponentByKind(
      libsbml::CoordinateKind_t::SPATIAL_COORDINATEKIND_CARTESIAN_Y);
  min = coord->getBoundaryMin();
  max = coord->getBoundaryMax();
  min->setValue(physicalOrigin.p.y());
  max->setValue(physicalOrigin.p.y() + physicalSize.height());
  SPDLOG_INFO("  - y now in range [{},{}]", min->getValue(), max->getValue());
  coord = geom->getCoordinateComponentByKind(
      libsbml::CoordinateKind_t::SPATIAL_COORDINATEKIND_CARTESIAN_Z);
  min = coord->getBoundaryMin();
  max = coord->getBoundaryMax();
  min->setValue(physicalOrigin.z);
  max->setValue(physicalOrigin.z + physicalSize.depth());
  SPDLOG_INFO("  - z now in range [{},{}]", min->getValue(), max->getValue());
  if (updateSBML) {
    updateCompartmentAndMembraneSizes();
  }
}

[[nodiscard]] const common::VolumeF &ModelGeometry::getVoxelSize() const {
  return voxelSize;
}

const common::VoxelF &ModelGeometry::getPhysicalOrigin() const {
  return physicalOrigin;
}

const common::VolumeF &ModelGeometry::getPhysicalSize() const {
  return physicalSize;
}

common::VoxelF
ModelGeometry::getPhysicalPoint(const common::Voxel &voxel) const {
  // returns physical location of *centre* of voxel
  if (!hasImage) {
    return {0.0, 0.0, 0.0};
  }
  common::VoxelF physicalPoint{physicalOrigin};
  physicalPoint.p.rx() +=
      voxelSize.width() * (static_cast<double>(voxel.p.x()) + 0.5);
  physicalPoint.p.ry() +=
      voxelSize.height() *
      (static_cast<double>(images[0].height() - 1 - voxel.p.y()) + 0.5);
  physicalPoint.z += voxelSize.depth() * (static_cast<double>(voxel.z) + 0.5);
  return physicalPoint;
}

QString
ModelGeometry::getPhysicalPointAsString(const common::Voxel &voxel) const {
  auto lengthUnit{modelUnits->getLength().name};
  auto physicalPoint{getPhysicalPoint(voxel)};
  return QString("x: %1 %2, y: %3 %2, z: %4 %2")
      .arg(physicalPoint.p.x())
      .arg(lengthUnit)
      .arg(physicalPoint.p.y())
      .arg(physicalPoint.z);
}

const common::ImageStack &ModelGeometry::getImages() const { return images; }

mesh::Mesh2d *ModelGeometry::getMesh2d() const { return mesh.get(); }

mesh::Mesh3d *ModelGeometry::getMesh3d() const { return mesh3d.get(); }

bool ModelGeometry::getIsValid() const { return isValid; }

bool ModelGeometry::getIsMeshValid() const { return isMeshValid; }

bool ModelGeometry::getHasImage() const { return hasImage; }

void ModelGeometry::updateGeometryImageColor(QRgb oldColor, QRgb newColor) {
  if (!hasImage) {
    SPDLOG_WARN("No image");
    return;
  }
  if (oldColor == newColor) {
    return;
  }
  auto colorTable = images.colorTable();
  auto colorIndex = static_cast<int>(colorTable.indexOf(oldColor));
  if (colorIndex < 0) {
    SPDLOG_WARN("oldColor {:x} not found in image", oldColor);
    return;
  }
  if (colorTable.indexOf(newColor) >= 0) {
    SPDLOG_ERROR("newColor {:x} already used in image", newColor);
    throw std::invalid_argument(
        "This color is already taken by another "
        "compartment. Please choose a different color.");
  }
  SPDLOG_INFO("Changing color {:x} to {:x} in geometry image", oldColor,
              newColor);
  images.setColor(colorIndex, newColor);
  sbmlAnnotation->sampledFieldColors = common::toStdVec(images.colorTable());
  modelCompartments->updateGeometryImageColor(oldColor, newColor);
  modelMembranes->updateGeometryImageColor(oldColor, newColor);
  modelMembranes->updateCompartments(modelCompartments->getCompartments());
  if (mesh3d != nullptr) {
    mesh3d->setColors(common::toStdVec(modelCompartments->getColors()),
                      modelMembranes->getIdColorPairs());
  }
  hasUnsavedChanges = true;
}

void ModelGeometry::writeGeometryToSBML() const {
  // todo: also export 3d mesh
  if (mesh != nullptr && mesh->isValid()) {
    sbmlAnnotation->meshParameters = {mesh->getBoundaryMaxPoints(),
                                      mesh->getCompartmentMaxTriangleArea(),
                                      mesh->getBoundarySimplificationType()};
  }
}

bool ModelGeometry::getHasUnsavedChanges() const { return hasUnsavedChanges; }

void ModelGeometry::setHasUnsavedChanges(bool unsavedChanges) {
  hasUnsavedChanges = unsavedChanges;
}

} // namespace sme::model
