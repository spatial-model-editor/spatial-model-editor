// Boundary detection
//  - constructBoundaries: generates a vector of Boundary objects from image
//  - Boundary class:
//     - contains an ordered set of points representing the boundary
//     - number of points can be updated to increase/reduce resolution
//     - if isMembrane, then provides a pair of boundaries separated by
//     membraneWidth
//  - BoundaryBoolGrid: utility class that identifies boundary pixels in image

#pragma once

#include <QImage>
#include <QPoint>
#include <array>
#include <vector>

namespace boundary {

constexpr std::size_t NULL_INDEX = std::numeric_limits<std::size_t>::max();

using QTriangleF = std::array<QPointF, 3>;
using ColourPair = std::pair<QRgb, QRgb>;

class BoundaryBoolGrid {
 private:
  std::vector<std::vector<bool>> grid;
  std::vector<std::vector<std::size_t>> fixedPointIndex;
  std::vector<std::vector<std::size_t>> membraneIndex;
  std::map<std::size_t, std::string> membraneNames;
  bool isBoundary(std::size_t x, std::size_t y) const;
  std::size_t getFixedPointIndex(std::size_t x, std::size_t y) const;
  void visitPoint(std::size_t x, std::size_t y);
  void setBoundaryPoint(const QPoint& point, bool multi,
                        std::size_t iMembrane = NULL_INDEX);
  QSize imageSize;
  QImage boundaryPixelsImage;

 public:
  std::size_t nPixels;
  const QSize& size() const;
  std::vector<QPoint> fixedPoints;
  std::vector<std::size_t> fixedPointCounter;
  bool isValid(const QPoint& point) const;
  bool isBoundary(const QPoint& point) const;
  bool isFixed(const QPoint& point) const;
  bool isMembrane(const QPoint& point) const;
  std::size_t getMembraneIndex(const QPoint& point) const;
  std::string getMembraneName(std::size_t membraneIndex) const;
  std::size_t getFixedPointIndex(const QPoint& point) const;
  const QPoint& getFixedPoint(const QPoint& point) const;
  void visitPoint(const QPoint& point);
  QImage getBoundaryPixelsImage() const;
  explicit BoundaryBoolGrid(
      const QImage& inputImage,
      const std::map<ColourPair, std::pair<std::size_t, std::string>>&
          mapColourPairToMembraneIndex = {},
      const std::vector<QRgb>& compartmentColours = {});
};

class Boundary {
 private:
  // full set of ordered vertices that make up the boundary
  std::vector<QPoint> vertices;
  // unit vector perpendicular to boundary
  std::vector<QPointF> normalUnitVectors;
  // vertex indices in reverse order of importance
  std::vector<std::size_t> orderedBoundaryIndices;
  std::size_t maxPoints;
  double membraneWidth = 1;
  void removeDegenerateVertices();
  void constructNormalUnitVectors();
  std::vector<std::size_t>::const_iterator smallestTrianglePointIndex(
      const std::vector<std::size_t>& pointIndices) const;

 public:
  bool isLoop;
  bool isMembrane;
  std::string membraneID;
  // approx to boundary using at most maxPoints
  std::vector<QPoint> points;
  std::vector<QPointF> outerPoints;
  std::size_t getMaxPoints() const;
  void setMaxPoints(std::size_t maxPoints = 12);
  double getMembraneWidth() const;
  void setMembraneWidth(double newMembraneWidth);
  explicit Boundary(const std::vector<QPoint>& boundaryPoints,
                    bool isClosedLoop = false,
                    bool isMembraneCompartment = false,
                    const std::string& membraneName = "");
};

std::pair<std::vector<Boundary>, QImage> constructBoundaries(
    const QImage& image,
    const std::map<ColourPair, std::pair<std::size_t, std::string>>&
        mapColourPairToMembraneIndex = {},
    const std::vector<QRgb>& compartmentColours = {});

}  // namespace boundary
