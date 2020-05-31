// Wrapper around Triangle meshing library
//   - https://www.cs.cmu.edu/~quake/triangle.html
//   - takes a set of boundary points & segments connecting them
//   - also an interior point for each compartment
//   - generates mesh with triangles labelled according to compartment
//   - replaces triangles not assigned a compartment with holes and re-meshes
//   - provides resulting vertices & triangles

#pragma once

#include <QPointF>
#include <array>
#include <string>
#include <vector>

namespace triangle {
struct triangulateio;
}

namespace mesh {

struct BoundarySegment {
  std::size_t start;
  std::size_t end;
};

using BoundarySegments = std::vector<BoundarySegment>;
using TriangleIndex = std::array<std::size_t, 3>;
using RectangleIndex = std::array<std::size_t, 4>;

struct Compartment {
  QPointF interiorPoint;
  double maxTriangleArea;
};

struct BoundaryProperties {
  std::size_t boundaryIndex;
  double width;
  bool isLoop;
  bool isMembrane;
  std::array<std::size_t, 2> innerFPIndices;
  std::array<std::size_t, 2> outerFPIndices;
};

struct TriangulateBoundaries {
  std::vector<QPointF> boundaryPoints;
  std::vector<BoundarySegments> boundaries;
  std::vector<Compartment> compartments;
  std::vector<BoundaryProperties> boundaryProperties;
};

struct TriangulateFixedPoints {
  std::size_t nFPs = 0;
  std::vector<QPointF> newFPs = {};
};

class Triangulate {
 private:
  std::vector<QPointF> points;
  std::vector<std::vector<TriangleIndex>> triangleIndices;
  std::vector<std::vector<RectangleIndex>> rectangleIndices;
  TriangulateBoundaries addMembranes(const TriangulateBoundaries& boundaries,
                                     const TriangulateFixedPoints& fixedPoints);
  void triangulateCompartments(const TriangulateBoundaries& boundaries);

 public:
  explicit Triangulate(const TriangulateBoundaries& boundaries,
                       const TriangulateFixedPoints& fixedPoints = {});
  const std::vector<QPointF>& getPoints() const;
  const std::vector<std::vector<TriangleIndex>>& getTriangleIndices() const;
  const std::vector<std::vector<RectangleIndex>>& getRectangleIndices() const;
};

}  // namespace mesh
