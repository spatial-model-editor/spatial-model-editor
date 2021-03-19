#include "geometry_parametric.hpp"
#include "logger.hpp"
#include "mesh.hpp"
#include "model_compartments.hpp"
#include "model_geometry.hpp"
#include "model_membranes.hpp"
#include "sbml_utils.hpp"
#include "utils.hpp"
#include "xml_annotation.hpp"
#include <QImage>
#include <memory>
#include <sbml/SBMLTypes.h>
#include <sbml/extension/SBMLDocumentPlugin.h>
#include <sbml/packages/spatial/common/SpatialExtensionTypes.h>
#include <sbml/packages/spatial/extension/SpatialExtension.h>

namespace sme {

namespace model {

const libsbml::ParametricGeometry *
getParametricGeometry(const libsbml::Geometry *geom) {
  if (geom == nullptr) {
    return nullptr;
  }
  for (unsigned i = 0; i < geom->getNumGeometryDefinitions(); ++i) {
    if (auto *def = geom->getGeometryDefinition(i);
        def->getIsActive() && def->isParametricGeometry()) {
      return static_cast<const libsbml::ParametricGeometry *>(def);
    }
  }
  return nullptr;
}

libsbml::ParametricGeometry *getParametricGeometry(libsbml::Geometry *geom) {
  if (geom == nullptr) {
    return nullptr;
  }
  for (unsigned i = 0; i < geom->getNumGeometryDefinitions(); ++i) {
    if (auto *def = geom->getGeometryDefinition(i);
        def->getIsActive() && def->isParametricGeometry()) {
      return static_cast<libsbml::ParametricGeometry *>(def);
    }
  }
  return nullptr;
}

const libsbml::ParametricObject *
getParametricObject(const libsbml::Model *model,
                    const std::string &compartmentID) {
  auto *geom = getGeometry(model);
  if (geom == nullptr) {
    return nullptr;
  }
  auto *comp = model->getCompartment(compartmentID);
  if (comp == nullptr) {
    return nullptr;
  }
  auto *scp = static_cast<const libsbml::SpatialCompartmentPlugin *>(
      comp->getPlugin("spatial"));
  const std::string domainTypeID =
      scp->getCompartmentMapping()->getDomainType();
  const auto *parageom = getParametricGeometry(geom);
  const auto *paraObj = parageom->getParametricObjectByDomainType(domainTypeID);
  return paraObj;
}

libsbml::ParametricObject *
getOrCreateParametricObject(libsbml::Model *model,
                            const std::string &compartmentID) {
  auto *geom = getOrCreateGeometry(model);
  if (geom == nullptr) {
    return nullptr;
  }
  auto *comp = model->getCompartment(compartmentID);
  if (comp == nullptr) {
    return nullptr;
  }
  auto *scp = static_cast<libsbml::SpatialCompartmentPlugin *>(
      comp->getPlugin("spatial"));
  const std::string domainTypeID =
      scp->getCompartmentMapping()->getDomainType();
  auto *parageom = getParametricGeometry(geom);
  auto *paraObj = parageom->getParametricObjectByDomainType(domainTypeID);
  if (paraObj == nullptr) {
    paraObj = parageom->createParametricObject();
    paraObj->setId(compartmentID + "_triangles");
    paraObj->setPolygonType(
        libsbml::PolygonKind_t::SPATIAL_POLYGONKIND_TRIANGLE);
    paraObj->setDomainType(domainTypeID);
    paraObj->setDataType(libsbml::DataKind_t::SPATIAL_DATAKIND_UINT32);
    paraObj->setCompression(
        libsbml::CompressionKind_t::SPATIAL_COMPRESSIONKIND_UNCOMPRESSED);
    SPDLOG_INFO("new parametricObject '{}'", paraObj->getId());
  }
  return paraObj;
}

std::vector<std::vector<QPointF>>
getInteriorPixelPoints(const ModelGeometry *modelGeometry,
                       const ModelCompartments *modelCompartments) {
  // get interiorPoints in terms of physical location
  // & convert them to integer pixel points
  // if any interior points are missing: return an empty vector
  std::vector<std::vector<QPointF>> interiorPoints;
  for (const auto &compartmentId : modelCompartments->getIds()) {
    auto interiorFloatsPhysical =
        modelCompartments->getInteriorPoints(compartmentId);
    if (!interiorFloatsPhysical) {
      return {};
    }
    SPDLOG_DEBUG("Found interior point:");
    auto &compartmentPoints = interiorPoints.emplace_back();
    for (const auto &floatPhysical : interiorFloatsPhysical.value()) {
      SPDLOG_DEBUG("  - physical location: ({},{})", floatPhysical.x(),
                   floatPhysical.y());
      QPointF interiorFloatPixel =
          (floatPhysical - modelGeometry->getPhysicalOrigin()) /
          modelGeometry->getPixelWidth();
      SPDLOG_DEBUG("  - pixel location: ({},{})", interiorFloatPixel.x(),
                   interiorFloatPixel.y());
      compartmentPoints.push_back(interiorFloatPixel);
    }
  }
  return interiorPoints;
}

std::unique_ptr<mesh::Mesh>
importParametricGeometryFromSBML(const libsbml::Model *model,
                                 const ModelGeometry *modelGeometry,
                                 const ModelCompartments *modelCompartments) {
  const auto *geom{getGeometry(model)};
  const auto *parageom{getParametricGeometry(geom)};
  if (parageom == nullptr) {
    SPDLOG_WARN("Failed to load Parametric Field geometry");
    return nullptr;
  }
  // get maxBoundaryPoints, maxTriangleAreas, membraneWidths
  if (auto meshParams{getMeshParamsAnnotationData(parageom)};
      meshParams.has_value()) {
    const auto &mp = meshParams.value();
    // generate Mesh
    SPDLOG_INFO("  - re-generating mesh");
    return std::make_unique<mesh::Mesh>(
        modelGeometry->getImage(), mp.maxPoints, mp.maxAreas,
        modelGeometry->getPixelWidth(), modelGeometry->getPhysicalOrigin(),
        utils::toStdVec(modelCompartments->getColours()));
  }
  SPDLOG_WARN("Failed to find mesh params annotation data");
  return nullptr;
}

void writeGeometryMeshToSBML(libsbml::Model *model, const mesh::Mesh *mesh,
                             const ModelCompartments &modelCompartments) {
  auto *geom = getOrCreateGeometry(model);
  auto *parageom = getParametricGeometry(geom);
  if (mesh == nullptr || !mesh->isValid()) {
    SPDLOG_INFO("No valid mesh to export to SBML");
    if (parageom != nullptr) {
      std::unique_ptr<libsbml::GeometryDefinition> pg(
          geom->removeGeometryDefinition(parageom->getId()));
      if (pg != nullptr) {
        SPDLOG_INFO("  - removed ParametricGeometry {}", pg->getId());
      }
      parageom = nullptr;
    }
    return;
  }
  if (parageom == nullptr) {
    SPDLOG_INFO("No ParametricGeometry found, creating...");
    parageom = geom->createParametricGeometry();
    parageom->setId("parametricGeometry");
    parageom->setIsActive(true);
    auto *sp = parageom->createSpatialPoints();
    sp->setId("spatialPoints");
    sp->setDataType(libsbml::DataKind_t::SPATIAL_DATAKIND_DOUBLE);
    sp->setCompression(
        libsbml::CompressionKind_t::SPATIAL_COMPRESSIONKIND_UNCOMPRESSED);
  }

  // add the parameters required
  // to reconstruct the mesh from the geometry image as an annotation
  addMeshParamsAnnotation(parageom, mesh);

  // write vertices
  std::vector<double> vertices = mesh->getVerticesAsFlatArray();
  auto *sp = parageom->getSpatialPoints();
  int sz = static_cast<int>(vertices.size());
  sp->setArrayData(vertices.data(), vertices.size());
  sp->setArrayDataLength(sz);
  SPDLOG_INFO("  - added {} doubles ({} vertices)", sz, sz / 2);

  SPDLOG_INFO(" Writing mesh triangles:");
  // write mesh triangles for each compartment
  for (int i = 0; i < modelCompartments.getIds().size(); ++i) {
    auto compartmentId = modelCompartments.getIds()[i].toStdString();
    SPDLOG_INFO("  - compartment {}", compartmentId);
    auto *po = getOrCreateParametricObject(model, compartmentId);
    if (po == nullptr) {
      SPDLOG_CRITICAL("    - no parametricObject found");
    }
    SPDLOG_INFO("    - parametricObject: {}", po->getId());
    std::vector<int> triangleInts =
        mesh->getTriangleIndicesAsFlatArray(static_cast<std::size_t>(i));
    int size = static_cast<int>(triangleInts.size());
    po->setPointIndexLength(size);
    po->setPointIndex(triangleInts.data(), triangleInts.size());
    SPDLOG_INFO("    - added {} uints ({} triangles)", size, size / 2);
  }
}

} // namespace model

} // namespace sme
