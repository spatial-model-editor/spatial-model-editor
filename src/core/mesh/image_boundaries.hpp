// ImageBoundaries

#pragma once

#include <QImage>
#include <QPoint>
#include <array>
#include <vector>

#include "boundary.hpp"

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

class ImageBoundaries {
 private:
  std::vector<Boundary> boundaries;
  std::vector<FixedPoint> fixedPoints;
  std::vector<QPointF> newFixedPoints;
  QImage boundaryPixelsImage;
  bool fpHasMembrane(const FixedPoint& fp) const;
  double getFpRadius(const FixedPoint& fp) const;
  void updateFpAngles(FixedPoint& fp);
  std::vector<double> getMidpointAngles(const FixedPoint& fp) const;
  void expandFP(FixedPoint& fp);
  void expandFPs();

 public:
  ImageBoundaries() = default;
  explicit ImageBoundaries(
      const QImage& image, const std::vector<QRgb>& compartmentColours = {},
      const std::vector<std::pair<std::string, ColourPair>>&
          membraneColourPairs = {});
  const std::vector<Boundary>& getBoundaries() const;
  const std::vector<FixedPoint>& getFixedPoints() const;
  const std::vector<QPointF>& getNewFixedPoints() const;
  const QImage& getBoundaryPixelsImage() const;
  void setMaxPoints(const std::vector<std::size_t>& boundaryMaxPoints);
  std::vector<std::size_t> setAutoMaxPoints();
  void setMaxPoints(std::size_t boundaryIndex, std::size_t maxPoints = 12);
  void setMembraneWidths(const std::vector<double>& newMembraneWidths);
  void setMembraneWidth(std::size_t boundaryIndex, double newMembraneWidth);
};

}  // namespace mesh
