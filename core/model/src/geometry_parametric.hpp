// SBML ParametricGeometry
//   - import/export fixed 3d mesh topology to/from SBML parametric geometry

#pragma once

#include "sme/gmsh.hpp"
#include "sme/mesh_types.hpp"
#include <QRgb>
#include <QString>
#include <QStringList>
#include <QVector>
#include <optional>
#include <utility>
#include <vector>

namespace libsbml {
class Geometry;
class Model;
} // namespace libsbml

namespace sme::mesh {
class Mesh2d;
class Mesh3d;
} // namespace sme::mesh

namespace sme::model {

struct GeometryParametricMesh {
  mesh::GMSHMesh mesh;
  std::vector<std::pair<QRgb, int>> colorTagPairs;
};

struct GeometryParametricMesh2d {
  std::vector<common::VoxelF> vertices;
  std::vector<std::vector<mesh::TriangulateTriangleIndex>> triangleIndices;
};

struct GeometryParametricImportResult {
  std::optional<GeometryParametricMesh> importedMesh{};
  std::optional<GeometryParametricMesh2d> importedMesh2d{};
  QString diagnostic{};
};

struct GeometryParametricExportResult {
  bool exported{false};
  QString diagnostic{};
};

/**
 * @brief Import fixed topology from SBML ParametricGeometry.
 *
 * For 3D, topology is expected as per-tetra face groups (12 indices per
 * tetrahedron). For 2D, topology is expected as triangle triplets (3 indices
 * per triangle).
 */
[[nodiscard]] GeometryParametricImportResult
importFixedMeshFromParametricGeometry(
    const libsbml::Model *model, const QStringList &compartmentIds,
    const QVector<QRgb> &compartmentColors,
    bool allowTriangleSurfaceFallback = false);

/**
 * @brief Export fixed 3d tetrahedral topology into SBML ParametricGeometry.
 */
[[nodiscard]] GeometryParametricExportResult
exportFixedMeshToParametricGeometry(libsbml::Model *model,
                                    const mesh::Mesh3d &mesh,
                                    const QStringList &compartmentIds,
                                    const QVector<QRgb> &compartmentColors);

/**
 * @brief Export fixed 2d triangular topology into SBML ParametricGeometry.
 */
[[nodiscard]] GeometryParametricExportResult
exportFixedMeshToParametricGeometry(libsbml::Model *model,
                                    const mesh::Mesh2d &mesh,
                                    const QStringList &compartmentIds,
                                    const QVector<QRgb> &compartmentColors);

} // namespace sme::model
