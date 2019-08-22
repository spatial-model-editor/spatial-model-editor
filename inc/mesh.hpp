// Meshing
//  - Mesh class:
//     - generates simplified boundary vertices and segments from image
//     - generates mesh from these points given an interior point for each
//     compartment
//     - outputs vertices and triangles in GMSH .msh format

#pragma once

#include <vector>

#include <QImage>
#include <QPoint>

#include "boundary.hpp"
#include "triangulate.hpp"

namespace mesh {

constexpr std::size_t NULL_INDEX = std::numeric_limits<std::size_t>::max();

using QTriangleF = std::array<QPointF, 3>;

class Mesh {
 private:
  bool readOnlyMesh = false;
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
  std::vector<boundary::Boundary> boundaries;
  std::vector<QPointF> vertices;
  std::size_t nTriangles;
  std::vector<std::vector<QTriangleF>> triangles;
  std::vector<std::vector<std::array<std::size_t, 3>>> triangleIndices;
  inline int pointToIndex(const QPoint& p) const {
    return p.x() + img.width() * p.y();
  }
  inline QPoint indexToPoint(int i) const {
    return QPoint(i % img.width(), i / img.width());
  }
  void constructMesh();

 public:
  Mesh() = default;
  explicit Mesh(const QImage& image,
                const std::vector<QPointF>& interiorPoints = {},
                const std::vector<std::size_t>& maxPoints = {},
                const std::vector<std::size_t>& maxTriangleArea = {});
  // constructor to load existing vertices&trianges without image
  Mesh(const std::vector<double>& inputVertices,
       const std::vector<std::vector<int>>& inputTriangleIndices,
       const std::vector<QPointF>& interiorPoints);
  // if mesh not constructed from image, but supplied as vertices&trianges,
  // we cannot alter boundary or triangle area easily, so treat as read-only
  bool isReadOnly() const;
  void setBoundaryMaxPoints(std::size_t boundaryIndex, std::size_t maxPoints);
  std::size_t getBoundaryMaxPoints(std::size_t boundaryIndex) const;
  std::vector<std::size_t> getBoundaryMaxPoints() const;
  void setCompartmentMaxTriangleArea(std::size_t compartmentIndex,
                                     std::size_t maxTriangleArea);
  std::size_t getCompartmentMaxTriangleArea(std::size_t compartmentIndex) const;
  const std::vector<std::size_t>& getCompartmentMaxTriangleArea() const;
  const std::vector<boundary::Boundary>& getBoundaries() const {
    return boundaries;
  }
  const std::vector<QPointF>& getVertices() const { return vertices; }
  const std::vector<std::vector<std::array<std::size_t, 3>>>&
  getTriangleIndices() const {
    return triangleIndices;
  }
  const std::vector<std::vector<QTriangleF>>& getTriangles() const {
    return triangles;
  }
  QImage getBoundariesImage(const QSize& size,
                            std::size_t boldBoundaryIndex) const;
  QImage getMeshImage(const QSize& size, std::size_t compartmentIndex) const;
  QString getGMSH(double pixelPhysicalSize = 1.0) const;
};

}  // namespace mesh
