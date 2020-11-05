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

namespace mesh {

struct BoundarySegment {
  std::size_t start;
  std::size_t end;
};

using BoundarySegments = std::vector<BoundarySegment>;
using TriangleIndex = std::array<std::size_t, 3>;
using RectangleIndex = std::array<std::size_t, 4>;

struct Compartment {
  std::vector<QPointF> interiorPoints;
  double maxTriangleArea;
};

struct BoundaryProperties {
  std::size_t boundaryIndex;
  bool isLoop;
  bool isMembrane;
  std::size_t membraneIndex;
};

struct TriangulateBoundaries {
  std::vector<QPointF> boundaryPoints;
  std::vector<BoundarySegments> boundaries;
  std::vector<Compartment> compartments;
  std::vector<BoundaryProperties> boundaryProperties;
};

class Triangulate {
private:
  std::vector<QPointF> points;
  std::vector<std::vector<TriangleIndex>> triangleIndices;
  void triangulateCompartments(const TriangulateBoundaries &boundaries);

public:
  explicit Triangulate(const TriangulateBoundaries &boundaries);
  const std::vector<QPointF> &getPoints() const;
  const std::vector<std::vector<TriangleIndex>> &getTriangleIndices() const;
};

} // namespace mesh
