// SBML ParametricGeometry
//   - import/export fixed 3d mesh topology to/from SBML parametric geometry

#pragma once

#include "sme/gmsh.hpp"
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
class Mesh3d;
}

namespace sme::model {

struct GeometryParametricMesh {
  mesh::GMSHMesh mesh;
  std::vector<std::pair<QRgb, int>> colorTagPairs;
};

struct GeometryParametricImportResult {
  std::optional<GeometryParametricMesh> importedMesh{};
  QString diagnostic{};
};

struct GeometryParametricExportResult {
  bool exported{false};
  QString diagnostic{};
};

/**
 * @brief Import fixed 3d tetrahedral topology from SBML ParametricGeometry.
 *
 * The topology is expected to be encoded as per-tetra face groups (12 indices
 * per tetrahedron) in each ParametricObject.
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

} // namespace sme::model
