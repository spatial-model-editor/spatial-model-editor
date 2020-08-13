// Some POD types used in meshing

#pragma once

#include <QPoint>
#include <QRgb>
#include <array>
#include <vector>

namespace mesh {

using QTriangleF = std::array<QPointF, 3>;
using ColourPair = std::pair<QRgb, QRgb>;

struct FPBoundaryLine {
  std::size_t boundaryIndex;
  bool startsFromFP;
  bool isMembrane;
  QPoint nearestPoint;
  double angle;
};

struct FixedPoint {
  QPoint point;
  std::vector<FPBoundaryLine> lines;
};

struct FpIndices {
  std::size_t startPoint;
  std::size_t endPoint;
};

} // namespace mesh
