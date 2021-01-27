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

std::size_t getOrInsertFPIndex(const QPoint &p,
                                      std::vector<QPoint> &fps);

struct TriangulateBoundaries {
  TriangulateBoundaries();
  TriangulateBoundaries(const std::vector<Boundary> &inputBoundaries,
                        const std::vector<std::vector<QPointF>> &interiorPoints,
                        const std::vector<std::size_t> &maxTriangleAreas);
  std::vector<QPointF> vertices;
  std::vector<TriangulateBoundarySegments> boundaries;
  std::vector<TriangulateCompartment> compartments;
};

} // namespace mesh

} // namespace sme
