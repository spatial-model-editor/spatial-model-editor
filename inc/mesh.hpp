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
  std::size_t defaultBoundaryMaxPoints = 12;
  std::size_t defaultCompartmentMaxTriangleArea = 40;
  std::vector<QPoint> nearestNeighbourDirectionPoints = {
      QPoint(1, 0), QPoint(-1, 0), QPoint(0, 1),  QPoint(0, -1),
      QPoint(1, 1), QPoint(1, -1), QPoint(-1, 1), QPoint(-1, -1)};
  // input data
  QImage img;
  std::vector<QPointF> compartmentInteriorPoints;
  std::vector<std::size_t> boundaryMaxPoints;
  std::vector<std::size_t> compartmentMaxTriangleArea;
  // output data
  std::vector<Boundary> fullBoundaries;
  std::vector<Boundary> boundaries;
  std::vector<QPointF> vertices;
  std::vector<std::vector<QTriangleF>> triangles;
  std::vector<std::array<std::size_t, 4>> triangleIDs;
  inline int pointToIndex(const QPoint& p) const {
    return p.x() + img.width() * p.y();
  }
  inline QPoint indexToPoint(int i) const {
    return QPoint(i % img.width(), i / img.width());
  }
  void simplifyBoundary(Boundary& boundaryPoints, std::size_t maxPoints) const;
  void updateBoundary(std::size_t boundaryIndex);
  void updateBoundaries();
  void constructFullBoundaries();
  void constructMesh();

 public:
  Mesh() = default;
  explicit Mesh(const QImage& image,
                const std::vector<QPointF>& interiorPoints = {},
                const std::vector<std::size_t>& maxPoints = {},
                const std::vector<std::size_t>& maxTriangleArea = {});
  void setBoundaryMaxPoints(std::size_t boundaryIndex, std::size_t maxPoints);
  std::size_t getBoundaryMaxPoints(std::size_t boundaryIndex) const;
  void setCompartmentMaxTriangleArea(std::size_t compartmentIndex,
                                     std::size_t maxTriangleArea);
  std::size_t getCompartmentMaxTriangleArea(std::size_t compartmentIndex) const;
  const std::vector<Boundary>& getBoundaries() const { return boundaries; }
  const std::vector<QPointF>& getVertices() const { return vertices; }
  const std::vector<std::vector<QTriangleF>>& getTriangles() const {
    return triangles;
  }
  QImage getBoundariesImage(const QSize& size,
                            std::size_t boldBoundaryIndex) const;
  QImage getMeshImage(const QSize& size, std::size_t compartmentIndex) const;
  QString getGMSH(double pixelPhysicalSize = 1.0) const;
};

}  // namespace mesh
