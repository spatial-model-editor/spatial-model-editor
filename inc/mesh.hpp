// Meshing
//  - Mesh class:
//     - generates boundary vertices and segments from image
//     - simplifies these boundary lines
//     - generates mesh from these points using the Triangle library
//       https://www.cs.cmu.edu/~quake/triangle.html
//     - outputs vertices and triangles in GMSH .msh format
//  - BoundaryBoolGrid: utility class that identifies boundary pixels in image

#pragma once

#include <vector>

#include <QImage>
#include <QPoint>

#include "triangle_wrapper.hpp"

namespace mesh {

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
  void unvisitPoint(std::size_t x, std::size_t y);

 public:
  std::vector<QPoint> fixedPoints;
  std::vector<std::size_t> fixedPointCounter;
  bool isBoundary(const QPoint& point) const;
  bool isFixed(const QPoint& point) const;
  std::size_t getFixedPointIndex(const QPoint& point) const;
  const QPoint& getFixedPoint(const QPoint& point) const;
  void setBoundaryPoint(const QPoint& point, bool multi = false);
  void visitPoint(const QPoint& point);
  void unvisitPoint(const QPoint& point);
  explicit BoundaryBoolGrid(const QImage& inputImage);
};

struct Boundary {
  bool isLoop = false;
  std::vector<QPoint> points;
};

class Mesh {
 private:
  const std::vector<QPoint> nearestNeighbourDirectionPoints = {
      QPoint(1, 0), QPoint(-1, 0), QPoint(0, 1),  QPoint(0, -1),
      QPoint(1, 1), QPoint(1, -1), QPoint(-1, 1), QPoint(-1, -1)};
  QImage img;
  std::vector<Boundary> boundaries;
  std::vector<QPointF> vertices;
  std::vector<std::vector<QTriangleF>> triangles;
  std::vector<std::array<std::size_t, 4>> triangleIDs;
  std::size_t maxPoints;
  inline int pointToIndex(const QPoint& p) const {
    return p.x() + img.width() * p.y();
  }
  inline QPoint indexToPoint(int i) const {
    return QPoint(i % img.width(), i / img.width());
  }
  void simplifyBoundary(Boundary& boundaryPoints) const;
  void constructBoundaries();
  void constructMesh(const std::vector<QPointF>& regions,
                     double maxTriangleArea);

 public:
  explicit Mesh(const QImage& image, const std::vector<QPointF>& regions = {},
                double maxTriangleArea = -1,
                std::size_t maxBoundaryPoints = 13);
  const std::vector<Boundary>& getBoundaries() const { return boundaries; }
  const std::vector<QPointF>& getVertices() const { return vertices; }
  const std::vector<std::vector<QTriangleF>>& getTriangles() const {
    return triangles;
  }
  QString getGMSH() const;
};

}  // namespace mesh
