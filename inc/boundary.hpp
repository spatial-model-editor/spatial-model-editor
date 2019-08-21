// Boundary detection
//  - constructBoundaries: generates a vector of Boundary objects from image
//  - Boundary class:
//     - contains an ordered set of points representing the boundary
//     - number of points can be updated to increase/reduce resolution
//  - BoundaryBoolGrid: utility class that identifies boundary pixels in image

#pragma once

#include <vector>

#include <QImage>
#include <QPoint>

namespace boundary {

constexpr std::size_t NULL_INDEX = std::numeric_limits<std::size_t>::max();

using QTriangleF = std::array<QPointF, 3>;

class BoundaryBoolGrid {
 private:
  std::vector<std::vector<bool>> grid;
  std::vector<std::vector<std::size_t>> fixedPointIndex;
  bool isBoundary(std::size_t x, std::size_t y) const;
  bool isFixed(std::size_t x, std::size_t y) const;
  std::size_t getFixedPointIndex(std::size_t x, std::size_t y) const;
  void visitPoint(std::size_t x, std::size_t y);

 public:
  std::vector<QPoint> fixedPoints;
  std::vector<std::size_t> fixedPointCounter;
  bool isBoundary(const QPoint& point) const;
  bool isFixed(const QPoint& point) const;
  std::size_t getFixedPointIndex(const QPoint& point) const;
  const QPoint& getFixedPoint(const QPoint& point) const;
  void setBoundaryPoint(const QPoint& point, bool multi = false);
  void visitPoint(const QPoint& point);
  explicit BoundaryBoolGrid(const QImage& inputImage);
};

class Boundary {
 private:
  // full set of ordered vertices that make up the boundary
  std::vector<QPoint> vertices;
  // vertex indices in reverse order of importance
  std::vector<std::size_t> orderedBoundaryIndices;
  std::size_t maxPoints;
  void removeDegenerateVertices();
  std::vector<std::size_t>::const_iterator smallestTrianglePointIndex(
      const std::vector<std::size_t>& pointIndices) const;

 public:
  bool isLoop;
  // approx to boundary using at most maxPoints
  std::vector<QPoint> points;
  std::size_t getMaxPoints() const;
  void setMaxPoints(std::size_t maxPoints = 12);
  explicit Boundary(const std::vector<QPoint>& boundaryPoints,
                    bool isClosedLoop = false);
};

std::vector<Boundary> constructBoundaries(const QImage& image);

}  // namespace boundary
