#pragma once

#include "boundary.hpp"
#include "sme/mesh_types.hpp"
#include <QPointF>
#include <array>
#include <cstddef>
#include <vector>

namespace sme::mesh {

/**
 * @brief Triangulate a set of boundary lines
 *
 * Given a set of boundary lines, a set of interior points for each compartment,
 * and a maximum allowed triangle area for each compartment, constructs a
 * Constrained Delauney Triangulation of the geometry, with the triangles
 * labelled according to the compartment they belong to.
 */
class Triangulate {
private:
  std::vector<QPointF> points;
  std::vector<std::vector<TriangulateTriangleIndex>> triangleIndices;

public:
  /**
   * @brief Triangulate a set of boundary lines
   *
   * Given a set of boundary lines, a set of interior points for each
   * compartment,
   * @param[in] boundaries the boundary lines separating the compartments
   * @param[in] interiorPoints the interior point(s) for each compartment
   * @param[in] maxTriangleAreas the maximum allowed triangle area for each
   *    compartment
   */
  explicit Triangulate(const std::vector<Boundary> &boundaries,
                       const std::vector<std::vector<QPointF>> &interiorPoints,
                       const std::vector<std::size_t> &maxTriangleAreas);
  /**
   * @brief The vertices or points in the mesh
   * @returns The vertices in the mesh
   */
  [[nodiscard]] const std::vector<QPointF> &getPoints() const;
  /**
   * @brief The triangle vertex indices
   *
   * For each compartment, the vertices of the triangles that define this
   * compartment in the mesh.
   *
   * @returns The triangle vertex indices
   */
  [[nodiscard]] const std::vector<std::vector<TriangulateTriangleIndex>> &
  getTriangleIndices() const;
};

} // namespace sme::mesh
