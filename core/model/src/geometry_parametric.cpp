#include "geometry_parametric.hpp"
#include "sbml_utils.hpp"
#include "sme/logger.hpp"
#include "sme/mesh2d.hpp"
#include "sme/mesh3d.hpp"
#include <algorithm>
#include <array>
#include <cmath>
#include <limits>
#include <map>
#include <memory>
#include <sbml/SBMLTypes.h>
#include <sbml/extension/SBMLDocumentPlugin.h>
#include <sbml/packages/spatial/common/SpatialExtensionTypes.h>
#include <sbml/packages/spatial/extension/SpatialExtension.h>
#include <set>
#include <string>

namespace sme::model {

namespace {

[[nodiscard]] const libsbml::ParametricGeometry *
getParametricGeometry(const libsbml::Geometry *geom) {
  if (geom == nullptr) {
    return nullptr;
  }
  for (unsigned i = 0; i < geom->getNumGeometryDefinitions(); ++i) {
    if (const auto *def = geom->getGeometryDefinition(i);
        def->getIsActive() && def->isParametricGeometry()) {
      return dynamic_cast<const libsbml::ParametricGeometry *>(def);
    }
  }
  return nullptr;
}

[[nodiscard]] libsbml::ParametricGeometry *
getParametricGeometry(libsbml::Geometry *geom) {
  return const_cast<libsbml::ParametricGeometry *>(
      getParametricGeometry(const_cast<const libsbml::Geometry *>(geom)));
}

[[nodiscard]] libsbml::ParametricGeometry *
getOrCreateParametricGeometry(libsbml::Geometry *geom) {
  if (auto *pg = getParametricGeometry(geom); pg != nullptr) {
    return pg;
  }
  auto *pg = geom->createParametricGeometry();
  pg->setId("parametricGeometry");
  pg->setIsActive(true);
  return pg;
}

[[nodiscard]] bool containsAllVertices(const std::array<std::size_t, 4> &tet,
                                       const std::array<std::size_t, 3> &face) {
  std::size_t count{0};
  for (const auto &f : face) {
    if (std::ranges::find(tet, f) != tet.cend()) {
      ++count;
    }
  }
  return count == 3;
}

[[nodiscard]] std::string getObjectName(const libsbml::ParametricObject *obj,
                                        unsigned i) {
  if (obj == nullptr || obj->getId().empty()) {
    return "index " + std::to_string(i);
  }
  return "'" + obj->getId() + "'";
}

template <typename TopologyByCompartment>
[[nodiscard]] bool isExportableCompartment(
    std::size_t iComp, const TopologyByCompartment &topologyByCompartment,
    const QStringList &compartmentIds, const QVector<QRgb> &compartmentColors) {
  return iComp < topologyByCompartment.size() &&
         iComp < static_cast<std::size_t>(compartmentIds.size()) &&
         iComp < static_cast<std::size_t>(compartmentColors.size()) &&
         !topologyByCompartment[iComp].empty() &&
         compartmentColors[static_cast<int>(iComp)] != 0;
}

template <typename TopologyByCompartment>
[[nodiscard]] bool
hasExportableCompartments(const TopologyByCompartment &topologyByCompartment,
                          const QStringList &compartmentIds,
                          const QVector<QRgb> &compartmentColors) {
  for (std::size_t iComp = 0; iComp < topologyByCompartment.size(); ++iComp) {
    if (isExportableCompartment(iComp, topologyByCompartment, compartmentIds,
                                compartmentColors)) {
      return true;
    }
  }
  return false;
}

void resetParametricGeometryObjects(libsbml::ParametricGeometry *pg) {
  while (pg->getNumParametricObjects() > 0) {
    std::unique_ptr<libsbml::ParametricObject> obj{
        pg->removeParametricObject(0)};
    if (obj != nullptr) {
      SPDLOG_INFO("Removing ParametricObject '{}'", obj->getId());
    }
  }
  if (pg->isSetSpatialPoints()) {
    pg->unsetSpatialPoints();
  }
  pg->setId("parametricGeometry");
  pg->setIsActive(true);
}

void setParametricGeometryPoints(libsbml::ParametricGeometry *pg,
                                 const std::vector<double> &vertices) {
  auto *points = pg->createSpatialPoints();
  points->setId("spatialPoints");
  points->setCompression(
      libsbml::CompressionKind_t::SPATIAL_COMPRESSIONKIND_UNCOMPRESSED);
  points->setDataType(libsbml::DataKind_t::SPATIAL_DATAKIND_DOUBLE);
  points->setArrayData(vertices);
  points->setArrayDataLength(static_cast<int>(vertices.size()));
}

[[nodiscard]] std::map<std::string, std::string, std::less<>>
getCompartmentDomainTypeMap(const libsbml::Model *model,
                            const QStringList &compartmentIds) {
  std::map<std::string, std::string, std::less<>> compartmentIdToDomainType;
  for (int i = 0; i < compartmentIds.size(); ++i) {
    const auto compartmentId = compartmentIds[i].toStdString();
    const auto *comp = model->getCompartment(compartmentId);
    if (comp == nullptr) {
      continue;
    }
    const auto *scp = static_cast<const libsbml::SpatialCompartmentPlugin *>(
        comp->getPlugin("spatial"));
    if (scp != nullptr && scp->isSetCompartmentMapping()) {
      compartmentIdToDomainType[compartmentId] =
          scp->getCompartmentMapping()->getDomainType();
    }
  }
  return compartmentIdToDomainType;
}

} // namespace

GeometryParametricImportResult importFixedMeshFromParametricGeometry(
    const libsbml::Model *model, const QStringList &compartmentIds,
    const QVector<QRgb> &compartmentColors, bool allowTriangleSurfaceFallback) {
  GeometryParametricImportResult result;
  const auto *geom = getGeometry(model);
  const auto *pg = getParametricGeometry(geom);
  if (pg == nullptr) {
    return result;
  }
  if (!pg->isSetSpatialPoints() || !pg->getSpatialPoints()->isSetArrayData()) {
    result.diagnostic =
        "Active ParametricGeometry is missing spatialPoints arrayData";
    return result;
  }

  const auto nCoords = geom == nullptr ? 0 : geom->getNumCoordinateComponents();
  if (nCoords != 2 && nCoords != 3) {
    result.diagnostic = QString("Unsupported ParametricGeometry dimensions: %1 "
                                "(only 2D/3D fixed-mesh import is supported)")
                            .arg(nCoords);
    return result;
  }

  std::vector<double> pointData;
  pg->getSpatialPoints()->getArrayData(pointData);
  if (pointData.size() < static_cast<std::size_t>(nCoords) ||
      pointData.size() % static_cast<std::size_t>(nCoords) != 0) {
    result.diagnostic =
        QString("ParametricGeometry spatialPoints length (%1) is invalid for "
                "%2D coordinates")
            .arg(pointData.size())
            .arg(nCoords);
    return result;
  }

  GeometryParametricMesh imported;
  GeometryParametricMesh2d imported2d;
  bool treatAs2d = nCoords == 2;
  if (nCoords == 3) {
    constexpr double planarTolerance{1e-12};
    const double referenceZ = pointData[2];
    treatAs2d = true;
    for (std::size_t i = 2; i < pointData.size(); i += 3) {
      if (std::abs(pointData[i] - referenceZ) > planarTolerance) {
        treatAs2d = false;
        break;
      }
    }
  }
  if (treatAs2d) {
    imported2d.vertices.reserve(pointData.size() /
                                static_cast<std::size_t>(nCoords));
    if (nCoords == 2) {
      for (std::size_t i = 0; i < pointData.size(); i += 2) {
        imported2d.vertices.emplace_back(pointData[i], pointData[i + 1], 0.0);
      }
    } else {
      for (std::size_t i = 0; i < pointData.size(); i += 3) {
        imported2d.vertices.emplace_back(pointData[i], pointData[i + 1], 0.0);
      }
    }
  } else {
    imported.mesh.vertices.reserve(pointData.size() / 3);
    for (std::size_t i = 0; i < pointData.size(); i += 3) {
      imported.mesh.vertices.emplace_back(pointData[i], pointData[i + 1],
                                          pointData[i + 2]);
    }
  }

  std::map<std::string, std::size_t, std::less<>> domainTypeToCompartmentIndex;
  for (int i = 0; i < compartmentIds.size(); ++i) {
    const auto id = compartmentIds[i].toStdString();
    if (const auto *comp = model->getCompartment(id);
        comp != nullptr && comp->getPlugin("spatial") != nullptr) {
      const auto *scp = static_cast<const libsbml::SpatialCompartmentPlugin *>(
          comp->getPlugin("spatial"));
      if (scp->isSetCompartmentMapping()) {
        domainTypeToCompartmentIndex[scp->getCompartmentMapping()
                                         ->getDomainType()] =
            static_cast<std::size_t>(i);
      }
    }
  }
  if (domainTypeToCompartmentIndex.empty()) {
    result.diagnostic =
        "No compartment domainType mappings found for fixed-mesh import";
    return result;
  }

  bool sawMappedObject{false};
  std::set<std::size_t> usedCompartmentIndices;
  for (unsigned i = 0; i < pg->getNumParametricObjects(); ++i) {
    const auto *obj = pg->getParametricObject(i);
    if (obj == nullptr) {
      continue;
    }
    const auto compIt = domainTypeToCompartmentIndex.find(obj->getDomainType());
    if (compIt == domainTypeToCompartmentIndex.cend()) {
      continue;
    }
    sawMappedObject = true;
    if (obj->getPolygonType() !=
        libsbml::PolygonKind_t::SPATIAL_POLYGONKIND_TRIANGLE) {
      result.diagnostic = QString("ParametricObject %1 uses unsupported "
                                  "polygonType %2 (expected TRIANGLE)")
                              .arg(getObjectName(obj, i).c_str())
                              .arg(obj->getPolygonType());
      return result;
    }

    const auto compartmentIndex = compIt->second;
    if (compartmentIndex >=
        static_cast<std::size_t>(compartmentColors.size())) {
      result.diagnostic = QString("ParametricObject %1 maps to out-of-range "
                                  "compartment index %2")
                              .arg(getObjectName(obj, i).c_str())
                              .arg(compartmentIndex);
      return result;
    }
    if (compartmentColors[static_cast<int>(compartmentIndex)] == 0) {
      continue;
    }

    std::vector<int> pointIndex;
    obj->getPointIndex(pointIndex);
    if (pointIndex.empty()) {
      result.diagnostic = QString("ParametricObject %1 has unsupported "
                                  "pointIndex length %2")
                              .arg(getObjectName(obj, i).c_str())
                              .arg(pointIndex.size());
      return result;
    }

    if (treatAs2d) {
      if (pointIndex.size() % 3 != 0) {
        result.diagnostic =
            QString("ParametricObject %1 has unsupported pointIndex length %2 "
                    "(expected 3*n entries)")
                .arg(getObjectName(obj, i).c_str())
                .arg(pointIndex.size());
        return result;
      }
      if (imported2d.triangleIndices.size() <
          static_cast<std::size_t>(compartmentColors.size())) {
        imported2d.triangleIndices.resize(compartmentColors.size());
      }
      auto &triangles = imported2d.triangleIndices[compartmentIndex];
      triangles.reserve(triangles.size() + pointIndex.size() / 3);
      for (std::size_t k = 0; k < pointIndex.size(); k += 3) {
        mesh::TriangulateTriangleIndex tri{};
        for (std::size_t j = 0; j < 3; ++j) {
          const auto idx = pointIndex[k + j];
          if (idx < 0 ||
              static_cast<std::size_t>(idx) >= imported2d.vertices.size()) {
            result.diagnostic =
                QString("ParametricObject %1 references out-of-range "
                        "spatialPoints index %2")
                    .arg(getObjectName(obj, i).c_str())
                    .arg(idx);
            return result;
          }
          tri[j] = static_cast<std::size_t>(idx);
        }
        if (tri[0] == tri[1] || tri[1] == tri[2] || tri[0] == tri[2]) {
          result.diagnostic =
              QString("ParametricObject %1 contains degenerate triangles")
                  .arg(getObjectName(obj, i).c_str());
          return result;
        }
        triangles.push_back(tri);
      }
      usedCompartmentIndices.insert(compartmentIndex);
      continue;
    }

    const int physicalTag = static_cast<int>(compartmentIndex) + 1;

    const auto importTriangleSurfaceAsTetraFan = [&]() -> bool {
      // Fallback for triangle-surface ParametricGeometry:
      // create a tetra fan from each unique surface triangle to the object
      // centroid so it can be voxelized into a compartment image.
      std::map<std::array<std::size_t, 3>, std::array<std::size_t, 3>>
          uniqueFaces;
      std::set<std::size_t> objectVertices;
      for (std::size_t k = 0; k < pointIndex.size(); k += 3) {
        std::array<std::size_t, 3> face{};
        for (std::size_t j = 0; j < 3; ++j) {
          const auto idx = pointIndex[k + j];
          if (idx < 0 ||
              static_cast<std::size_t>(idx) >= imported.mesh.vertices.size()) {
            result.diagnostic =
                QString("ParametricObject %1 references out-of-range "
                        "spatialPoints index %2")
                    .arg(getObjectName(obj, i).c_str())
                    .arg(idx);
            return false;
          }
          face[j] = static_cast<std::size_t>(idx);
        }
        auto faceKey = face;
        std::ranges::sort(faceKey);
        if (faceKey[0] == faceKey[1] || faceKey[1] == faceKey[2]) {
          result.diagnostic =
              QString("ParametricObject %1 contains degenerate triangle faces")
                  .arg(getObjectName(obj, i).c_str());
          return false;
        }
        if (!uniqueFaces.contains(faceKey)) {
          uniqueFaces.emplace(faceKey, face);
        }
        objectVertices.insert(faceKey[0]);
        objectVertices.insert(faceKey[1]);
        objectVertices.insert(faceKey[2]);
      }
      if (uniqueFaces.empty() || objectVertices.size() < 3) {
        result.diagnostic =
            QString("ParametricObject %1 has insufficient valid triangles for "
                    "surface fallback import")
                .arg(getObjectName(obj, i).c_str());
        return false;
      }
      sme::common::VoxelF centroid{};
      for (auto idx : objectVertices) {
        const auto &v = imported.mesh.vertices[idx];
        centroid.p.rx() += v.p.x();
        centroid.p.ry() += v.p.y();
        centroid.z += v.z;
      }
      const auto invN = 1.0 / static_cast<double>(objectVertices.size());
      centroid.p.rx() *= invN;
      centroid.p.ry() *= invN;
      centroid.z *= invN;
      const auto centroidIndex = imported.mesh.vertices.size();
      imported.mesh.vertices.push_back(centroid);
      const auto tetraVolumeAbs = [&imported](const auto &faceIndices,
                                              std::size_t apexIndex) {
        const auto &a = imported.mesh.vertices[faceIndices[0]];
        const auto &b = imported.mesh.vertices[faceIndices[1]];
        const auto &c = imported.mesh.vertices[faceIndices[2]];
        const auto &d = imported.mesh.vertices[apexIndex];
        const auto bax = b.p.x() - a.p.x();
        const auto bay = b.p.y() - a.p.y();
        const auto baz = b.z - a.z;
        const auto cax = c.p.x() - a.p.x();
        const auto cay = c.p.y() - a.p.y();
        const auto caz = c.z - a.z;
        const auto dax = d.p.x() - a.p.x();
        const auto day = d.p.y() - a.p.y();
        const auto daz = d.z - a.z;
        const auto cx = cay * daz - caz * day;
        const auto cy = caz * dax - cax * daz;
        const auto cz = cax * day - cay * dax;
        const auto det = bax * cx + bay * cy + baz * cz;
        return std::abs(det) / 6.0;
      };
      constexpr double minTetraVolume{1e-15};

      const auto addTetraFanForApex = [&](std::size_t apexIndex) {
        std::size_t nAdded{0};
        for (const auto &[faceKey, face] : uniqueFaces) {
          if (tetraVolumeAbs(faceKey, apexIndex) <= minTetraVolume) {
            continue;
          }
          imported.mesh.tetrahedra.push_back(
              {{{face[0], face[1], face[2], apexIndex}}, physicalTag});
          ++nAdded;
        }
        return nAdded;
      };

      std::size_t nAddedTetrahedra = addTetraFanForApex(centroidIndex);
      if (nAddedTetrahedra == 0) {
        const auto &seedFace = uniqueFaces.cbegin()->second;
        const auto &a = imported.mesh.vertices[seedFace[0]];
        const auto &b = imported.mesh.vertices[seedFace[1]];
        const auto &c = imported.mesh.vertices[seedFace[2]];
        const auto abx = b.p.x() - a.p.x();
        const auto aby = b.p.y() - a.p.y();
        const auto abz = b.z - a.z;
        const auto acx = c.p.x() - a.p.x();
        const auto acy = c.p.y() - a.p.y();
        const auto acz = c.z - a.z;
        const auto nx = aby * acz - abz * acy;
        const auto ny = abz * acx - abx * acz;
        const auto nz = abx * acy - aby * acx;
        const auto normalNorm = std::sqrt(nx * nx + ny * ny + nz * nz);
        if (normalNorm <= std::numeric_limits<double>::epsilon()) {
          result.diagnostic =
              QString("ParametricObject %1 surface fallback could not compute "
                      "a non-zero face normal")
                  .arg(getObjectName(obj, i).c_str());
          return false;
        }

        double minX = std::numeric_limits<double>::max();
        double minY = std::numeric_limits<double>::max();
        double minZ = std::numeric_limits<double>::max();
        double maxX = std::numeric_limits<double>::lowest();
        double maxY = std::numeric_limits<double>::lowest();
        double maxZ = std::numeric_limits<double>::lowest();
        for (const auto idx : objectVertices) {
          const auto &v = imported.mesh.vertices[idx];
          minX = std::min(minX, v.p.x());
          minY = std::min(minY, v.p.y());
          minZ = std::min(minZ, v.z);
          maxX = std::max(maxX, v.p.x());
          maxY = std::max(maxY, v.p.y());
          maxZ = std::max(maxZ, v.z);
        }
        const auto bboxDiagonal = std::sqrt((maxX - minX) * (maxX - minX) +
                                            (maxY - minY) * (maxY - minY) +
                                            (maxZ - minZ) * (maxZ - minZ));
        const auto offset = std::max(1e-9, 1e-6 * bboxDiagonal);
        auto &apex = imported.mesh.vertices[centroidIndex];
        apex.p.rx() += offset * nx / normalNorm;
        apex.p.ry() += offset * ny / normalNorm;
        apex.z += offset * nz / normalNorm;
        nAddedTetrahedra = addTetraFanForApex(centroidIndex);
      }

      if (nAddedTetrahedra == 0) {
        result.diagnostic =
            QString("ParametricObject %1 surface fallback produced only "
                    "degenerate tetrahedra")
                .arg(getObjectName(obj, i).c_str());
        return false;
      }
      usedCompartmentIndices.insert(compartmentIndex);
      return true;
    };

    if (pointIndex.size() % 12 != 0) {
      if (!allowTriangleSurfaceFallback || pointIndex.size() % 3 != 0) {
        result.diagnostic =
            QString("ParametricObject %1 has unsupported pointIndex length %2 "
                    "(expected 12*n entries)")
                .arg(getObjectName(obj, i).c_str())
                .arg(pointIndex.size());
        return result;
      }
      if (!importTriangleSurfaceAsTetraFan()) {
        return result;
      }
      continue;
    }

    std::vector<mesh::GMSHTetrahedron> objectTetrahedra;
    objectTetrahedra.reserve(pointIndex.size() / 12);
    QString tetraImportDiagnostic;
    bool tetraImportOk{true};
    for (std::size_t k = 0; k < pointIndex.size(); k += 12) {
      std::array<std::size_t, 4> tet{};
      std::set<std::size_t> vertices;
      std::set<std::array<std::size_t, 3>> uniqueFaces;
      std::array<std::array<std::size_t, 3>, 4> faces{};
      for (std::size_t f = 0; f < 4; ++f) {
        std::array<std::size_t, 3> face{};
        for (std::size_t j = 0; j < 3; ++j) {
          const auto idx = pointIndex[k + 3 * f + j];
          if (idx < 0 ||
              static_cast<std::size_t>(idx) >= imported.mesh.vertices.size()) {
            tetraImportDiagnostic =
                QString("ParametricObject %1 references out-of-range "
                        "spatialPoints index %2")
                    .arg(getObjectName(obj, i).c_str())
                    .arg(idx);
            tetraImportOk = false;
            break;
          }
          face[j] = static_cast<std::size_t>(idx);
        }
        if (!tetraImportOk) {
          break;
        }
        std::ranges::sort(face);
        if (face[0] == face[1] || face[1] == face[2]) {
          tetraImportDiagnostic =
              QString("ParametricObject %1 contains degenerate triangle faces")
                  .arg(getObjectName(obj, i).c_str());
          tetraImportOk = false;
          break;
        }
        if (!uniqueFaces.insert(face).second) {
          tetraImportDiagnostic =
              QString("ParametricObject %1 contains duplicate triangle faces")
                  .arg(getObjectName(obj, i).c_str());
          tetraImportOk = false;
          break;
        }
        faces[f] = face;
        vertices.insert(face[0]);
        vertices.insert(face[1]);
        vertices.insert(face[2]);
      }
      if (!tetraImportOk) {
        break;
      }
      if (vertices.size() != 4) {
        tetraImportDiagnostic =
            QString("ParametricObject %1 does not encode tetrahedra over "
                    "exactly 4 unique vertices")
                .arg(getObjectName(obj, i).c_str());
        tetraImportOk = false;
        break;
      }
      std::size_t n{0};
      for (auto v : vertices) {
        tet[n] = v;
        ++n;
      }
      const bool facesMatch =
          std::ranges::all_of(faces, [&tet](const auto &face) {
            return containsAllVertices(tet, face);
          });
      if (!facesMatch) {
        tetraImportDiagnostic =
            QString("ParametricObject %1 contains faces that do not form a "
                    "valid tetrahedron")
                .arg(getObjectName(obj, i).c_str());
        tetraImportOk = false;
        break;
      }
      objectTetrahedra.push_back({tet, physicalTag});
    }
    if (!tetraImportOk) {
      if (!allowTriangleSurfaceFallback || pointIndex.size() % 3 != 0) {
        result.diagnostic = tetraImportDiagnostic;
        return result;
      }
      if (!importTriangleSurfaceAsTetraFan()) {
        if (result.diagnostic.isEmpty()) {
          result.diagnostic = tetraImportDiagnostic;
        }
        return result;
      }
      continue;
    }
    imported.mesh.tetrahedra.insert(imported.mesh.tetrahedra.end(),
                                    objectTetrahedra.cbegin(),
                                    objectTetrahedra.cend());
    usedCompartmentIndices.insert(compartmentIndex);
  }

  if (!sawMappedObject) {
    result.diagnostic =
        "No ParametricObject entries matched compartment domainType mappings";
    return result;
  }
  if (treatAs2d) {
    std::size_t nTriangles{0};
    for (const auto &compTriangles : imported2d.triangleIndices) {
      nTriangles += compTriangles.size();
    }
    if (nTriangles == 0) {
      result.diagnostic = "ParametricGeometry contained no valid triangles for "
                          "fixed-mesh import";
      return result;
    }
    result.importedMesh2d = std::move(imported2d);
    return result;
  }
  if (imported.mesh.tetrahedra.empty()) {
    result.diagnostic = "ParametricGeometry contained no valid tetrahedra for "
                        "fixed-mesh import";
    return result;
  }

  imported.colorTagPairs.reserve(usedCompartmentIndices.size());
  for (const auto i : usedCompartmentIndices) {
    imported.colorTagPairs.emplace_back(compartmentColors[static_cast<int>(i)],
                                        static_cast<int>(i) + 1);
  }
  result.importedMesh = std::move(imported);
  return result;
}

GeometryParametricExportResult exportFixedMeshToParametricGeometry(
    libsbml::Model *model, const mesh::Mesh3d &mesh,
    const QStringList &compartmentIds, const QVector<QRgb> &compartmentColors) {
  GeometryParametricExportResult result;
  auto *geom = getOrCreateGeometry(model);
  if (geom == nullptr) {
    result.diagnostic =
        "Failed to export fixed mesh: no SBML Geometry available";
    return result;
  }
  if (geom->getNumCoordinateComponents() != 3) {
    result.diagnostic =
        QString(
            "Failed to export fixed mesh: expected 3 coordinate components, "
            "found %1")
            .arg(geom->getNumCoordinateComponents());
    return result;
  }

  auto *pg = getOrCreateParametricGeometry(geom);
  if (pg == nullptr) {
    result.diagnostic =
        "Failed to export fixed mesh: unable to create ParametricGeometry";
    return result;
  }

  const auto &tetrahedra = mesh.getTetrahedronIndices();
  if (!hasExportableCompartments(tetrahedra, compartmentIds,
                                 compartmentColors)) {
    result.diagnostic = "Failed to export fixed mesh: no tetrahedra found for "
                        "exportable compartments";
    return result;
  }

  resetParametricGeometryObjects(pg);
  const auto vertices = mesh.getVerticesAsFlatArray();
  setParametricGeometryPoints(pg, vertices);
  const auto compartmentIdToDomainType =
      getCompartmentDomainTypeMap(model, compartmentIds);

  for (std::size_t iComp = 0; iComp < tetrahedra.size(); ++iComp) {
    if (!isExportableCompartment(iComp, tetrahedra, compartmentIds,
                                 compartmentColors)) {
      continue;
    }

    const auto compartmentId =
        compartmentIds[static_cast<int>(iComp)].toStdString();
    const auto domainIt = compartmentIdToDomainType.find(compartmentId);
    if (domainIt == compartmentIdToDomainType.cend()) {
      result.diagnostic =
          QString("Failed to export fixed mesh: missing domainType mapping for "
                  "compartment %1")
              .arg(compartmentId.c_str());
      return result;
    }

    std::vector<int> indices;
    indices.reserve(tetrahedra[iComp].size() * 12);
    for (const auto &tet : tetrahedra[iComp]) {
      const auto a = static_cast<int>(tet[0]);
      const auto b = static_cast<int>(tet[1]);
      const auto c = static_cast<int>(tet[2]);
      const auto d = static_cast<int>(tet[3]);
      // Encode each tetrahedron as 4 triangle faces.
      indices.insert(indices.end(), {a, b, c, a, b, d, a, c, d, b, c, d});
    }

    auto *obj = pg->createParametricObject();
    obj->setId(compartmentId + "_tetra_faces");
    obj->setPolygonType(libsbml::PolygonKind_t::SPATIAL_POLYGONKIND_TRIANGLE);
    obj->setDomainType(domainIt->second);
    obj->setCompression(
        libsbml::CompressionKind_t::SPATIAL_COMPRESSIONKIND_UNCOMPRESSED);
    obj->setDataType(libsbml::DataKind_t::SPATIAL_DATAKIND_UINT32);
    obj->setPointIndex(indices);
    obj->setPointIndexLength(static_cast<int>(indices.size()));
  }

  result.exported = true;
  return result;
}

GeometryParametricExportResult exportFixedMeshToParametricGeometry(
    libsbml::Model *model, const mesh::Mesh2d &mesh,
    const QStringList &compartmentIds, const QVector<QRgb> &compartmentColors) {
  GeometryParametricExportResult result;
  auto *geom = getOrCreateGeometry(model);
  if (geom == nullptr) {
    result.diagnostic =
        "Failed to export fixed mesh: no SBML Geometry available";
    return result;
  }
  const auto nCoords = geom->getNumCoordinateComponents();
  if (nCoords != 2 && nCoords != 3) {
    result.diagnostic =
        QString("Failed to export fixed mesh: expected 2 or 3 coordinate "
                "components, "
                "found %1")
            .arg(nCoords);
    return result;
  }

  auto *pg = getOrCreateParametricGeometry(geom);
  if (pg == nullptr) {
    result.diagnostic =
        "Failed to export fixed mesh: unable to create ParametricGeometry";
    return result;
  }

  const auto &triangles = mesh.getTriangleIndices();
  if (!hasExportableCompartments(triangles, compartmentIds,
                                 compartmentColors)) {
    result.diagnostic = "Failed to export fixed mesh: no triangles found for "
                        "exportable compartments";
    return result;
  }

  resetParametricGeometryObjects(pg);
  auto vertices = mesh.getVerticesAsFlatArray();
  if (nCoords == 3) {
    double zValue{0.0};
    if (const auto *zcoord = geom->getCoordinateComponentByKind(
            libsbml::CoordinateKind_t::SPATIAL_COORDINATEKIND_CARTESIAN_Z);
        zcoord != nullptr && zcoord->isSetBoundaryMin()) {
      zValue = zcoord->getBoundaryMin()->getValue();
    }
    std::vector<double> vertices3d;
    vertices3d.reserve(vertices.size() / 2 * 3);
    for (std::size_t i = 0; i + 1 < vertices.size(); i += 2) {
      vertices3d.push_back(vertices[i]);
      vertices3d.push_back(vertices[i + 1]);
      vertices3d.push_back(zValue);
    }
    vertices = std::move(vertices3d);
  }
  setParametricGeometryPoints(pg, vertices);
  const auto compartmentIdToDomainType =
      getCompartmentDomainTypeMap(model, compartmentIds);

  for (std::size_t iComp = 0; iComp < triangles.size(); ++iComp) {
    if (!isExportableCompartment(iComp, triangles, compartmentIds,
                                 compartmentColors)) {
      continue;
    }

    const auto compartmentId =
        compartmentIds[static_cast<int>(iComp)].toStdString();
    const auto domainIt = compartmentIdToDomainType.find(compartmentId);
    if (domainIt == compartmentIdToDomainType.cend()) {
      result.diagnostic =
          QString("Failed to export fixed mesh: missing domainType mapping for "
                  "compartment %1")
              .arg(compartmentId.c_str());
      return result;
    }

    const auto indices = mesh.getTriangleIndicesAsFlatArray(iComp);
    auto *obj = pg->createParametricObject();
    obj->setId(compartmentId + "_triangles");
    obj->setPolygonType(libsbml::PolygonKind_t::SPATIAL_POLYGONKIND_TRIANGLE);
    obj->setDomainType(domainIt->second);
    obj->setCompression(
        libsbml::CompressionKind_t::SPATIAL_COMPRESSIONKIND_UNCOMPRESSED);
    obj->setDataType(libsbml::DataKind_t::SPATIAL_DATAKIND_UINT32);
    obj->setPointIndex(indices);
    obj->setPointIndexLength(static_cast<int>(indices.size()));
  }

  result.exported = true;
  return result;
}

} // namespace sme::model
