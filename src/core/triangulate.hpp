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

namespace triangulate {

using SegmentIndexPair = std::array<std::size_t, 2>;
using BoundarySegments = std::vector<SegmentIndexPair>;
using TriangleIndex = std::array<std::size_t, 3>;
using RectangleIndex = std::array<std::size_t, 4>;

struct Compartment {
  QPointF interiorPoint;
  double maxTriangleArea;
  Compartment() = default;
  Compartment(const QPointF& point, double area)
      : interiorPoint(point), maxTriangleArea(area) {}
};

struct Membrane {
  std::size_t boundaryIndex;
  double width;
  Membrane() = default;
  Membrane(std::size_t index, double w) : boundaryIndex(index), width(w) {}
};

class Triangulate {
 private:
  // http://www.cs.cmu.edu/~quake/triangle.switch.html
  //  - Q: no printf output
  //       (use V, VV, VVV for more verbose output)
  //  - z: 0-based indexing
  //  - p: supply Planar Straight Line Graph with vertices, segments, etc
  //  - j: remove unused vertices from output
  //       NOTE not doing this as it may change indices of existing points
  //  - q: generate quality triangles
  //       (e.g. q20 adds vertices such that triangle angles are between 20 and
  //       180-2*20 degrees)
  //  - A: regional attribute (identify compartment for each triangle)
  //  - a: max triangle area for each compartment
  const std::string triangleFlags = "Qzpq20.5Aa";
  std::vector<QPointF> points;
  std::vector<std::vector<TriangleIndex>> triangleIndices;
  std::vector<std::vector<RectangleIndex>> rectangleIndices;
  void setPointList(triangle::triangulateio& in,
                    const std::vector<QPointF>& boundaryPoints) const;
  void setSegmentList(triangle::triangulateio& in,
                      const std::vector<BoundarySegments>& boundaries) const;
  void setRegionList(triangle::triangulateio& in,
                     const std::vector<Compartment>& compartments) const;
  void setHoleList(triangle::triangulateio& in,
                   const std::vector<QPointF>& holes) const;
  // construct pointlist and segmentlist from supplied boundaries - may add
  // Steiner points to supplied boundary points if required for mesh quality
  triangle::triangulateio triangulateBoundaries(
      const std::vector<QPointF>& boundaryPoints,
      const std::vector<BoundarySegments>& boundaries,
      const std::vector<Compartment>& compartments,
      const std::vector<Membrane>& membranes);
  // triangulate compartments without altering any boundary segments
  void triangulateCompartments(triangle::triangulateio& in);

 public:
  Triangulate(const std::vector<QPointF>& boundaryPoints,
              const std::vector<BoundarySegments>& boundaries,
              const std::vector<Compartment>& compartments,
              const std::vector<Membrane>& membranes);
  const std::vector<QPointF>& getPoints() const;
  const std::vector<std::vector<TriangleIndex>>& getTriangleIndices() const;
  const std::vector<std::vector<RectangleIndex>>& getRectangleIndices() const;
};

}  // namespace triangulate
