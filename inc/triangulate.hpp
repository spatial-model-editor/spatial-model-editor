// Wrapper around Triangle meshing library
//   - https://www.cs.cmu.edu/~quake/triangle.html
//   - takes a set of boundary points & segments connecting them
//   - also an interior point for each compartment
//   - generates mesh with triangles labelled according to compartment
//   - replaces triangles not assigned a compartment with holes and re-meshes
//   - provides resulting vertices & triangles

#pragma once

#include <array>
#include <string>
#include <vector>

#include <QPoint>
#include <QPointF>

#include "triangle/triangle.hpp"

namespace triangulate {

using SegmentIndexPair = std::array<std::size_t, 2>;
using BoundarySegments = std::vector<SegmentIndexPair>;
using TriangleIndex = std::array<std::size_t, 4>;

struct Compartment {
  QPointF interiorPoint;
  double maxTriangleArea;
  Compartment() = default;
  Compartment(const QPointF& point, double area)
      : interiorPoint(point), maxTriangleArea(area) {}
};

class Triangulate {
 private:
  std::vector<QPointF> points;
  std::vector<TriangleIndex> triangleIndices;

  void setPointList(triangle::triangulateio& in,
                    const std::vector<QPoint>& boundaryPoints) const;
  void setSegmentList(triangle::triangulateio& in,
                      const std::vector<BoundarySegments>& boundaries) const;
  void setRegionList(triangle::triangulateio& in,
                     const std::vector<Compartment>& compartments) const;
  void setHoleList(triangle::triangulateio& in,
                   const std::vector<QPointF>& holes) const;

 public:
  Triangulate(const std::vector<QPoint>& boundaryPoints,
              const std::vector<BoundarySegments>& boundaries,
              const std::vector<Compartment>& compartments);
  const std::vector<QPointF>& getPoints() const;
  const std::vector<TriangleIndex>& getTriangleIndices() const;
};

}  // namespace triangulate
