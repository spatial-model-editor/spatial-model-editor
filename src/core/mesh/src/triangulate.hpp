// Wrapper around Triangle meshing library
//   - https://www.cs.cmu.edu/~quake/triangle.html
//   - takes a set of boundary points & segments connecting them
//   - also a set of interior points for each compartment
//   - generates mesh with triangles labelled according to compartment
//   - replaces triangles not assigned a compartment with holes and re-meshes
//   - provides resulting vertices & triangles

#pragma once

#include <QPointF>
#include <algorithm>
#include <array>
#include <cstddef>
#include <string>
#include <vector>

namespace sme {

namespace mesh {

class Boundary;

struct TriangulateBoundarySegment {
  std::size_t start;
  std::size_t end;
};

using TriangulateBoundarySegments = std::vector<TriangulateBoundarySegment>;
using TriangulateTriangleIndex = std::array<std::size_t, 3>;

struct TriangulateCompartment {
  std::vector<QPointF> interiorPoints;
  double maxTriangleArea;
};

struct TriangulateBoundaries {
  TriangulateBoundaries();
  TriangulateBoundaries(const std::vector<Boundary> &inputBoundaries,
                        const std::vector<std::vector<QPointF>> &interiorPoints,
                        const std::vector<std::size_t> &maxTriangleAreas);
  std::vector<QPointF> vertices;
  std::vector<TriangulateBoundarySegments> boundaries;
  std::vector<TriangulateCompartment> compartments;
};

class Triangulate {
private:
  std::vector<QPointF> points;
  std::vector<std::vector<TriangulateTriangleIndex>> triangleIndices;

public:
  explicit Triangulate(const TriangulateBoundaries &boundaries);
  [[nodiscard]] const std::vector<QPointF> &getPoints() const;
  [[nodiscard]] const std::vector<std::vector<TriangulateTriangleIndex>> &
  getTriangleIndices() const;
};

} // namespace mesh

} // namespace sme
