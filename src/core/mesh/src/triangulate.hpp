// Wrapper around a meshing library
//   - takes boundaries, interior point & max area for each compartment
//   - generates mesh with triangles labelled according to compartment
//   - provides resulting vertices & triangles

#pragma once

#include <QPointF>
#include <algorithm>
#include <array>
#include <cstddef>
#include <string>
#include <vector>
#include "boundary.hpp"

namespace sme {

namespace mesh {

using TriangulateTriangleIndex = std::array<std::size_t, 3>;

class Triangulate {
private:
  std::vector<QPointF> points;
  std::vector<std::vector<TriangulateTriangleIndex>> triangleIndices;

public:
  explicit Triangulate(const std::vector<Boundary> &inputBoundaries,
                       const std::vector<std::vector<QPointF>> &interiorPoints,
                       const std::vector<std::size_t> &maxTriangleAreas);
  [[nodiscard]] const std::vector<QPointF> &getPoints() const;
  [[nodiscard]] const std::vector<std::vector<TriangulateTriangleIndex>> &
  getTriangleIndices() const;
};

} // namespace mesh

} // namespace sme
