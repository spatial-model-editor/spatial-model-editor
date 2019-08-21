// Wrapper around Triangle meshing library
//   - https://www.cs.cmu.edu/~quake/triangle.html

#pragma once

#include <array>
#include <string>
#include <vector>

#include <QPoint>
#include <QPointF>

#include "triangle/triangle.hpp"

namespace triangle_wrapper {

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
  triangle::triangulateio in;
  std::vector<QPointF> points;
  std::vector<TriangleIndex> triangleIndices;

  void setPointList(const std::vector<QPoint>& boundaryPoints);
  void setSegmentList(const std::vector<BoundarySegments>& boundaries);
  void setRegionList(const std::vector<Compartment>& compartments);
  void setHoleList(const std::vector<QPointF>& holes);

 public:
  Triangulate(const std::vector<QPoint>& boundaryPoints,
              const std::vector<BoundarySegments>& boundaries,
              const std::vector<Compartment>& compartments);
  const std::vector<QPointF>& getPoints() const;
  const std::vector<TriangleIndex>& getTriangleIndices() const;
};

}  // namespace triangle_wrapper
