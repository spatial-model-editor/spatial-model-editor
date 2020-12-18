// Meshing
//  - Mesh class:
//     - generates simplified boundary vertices and segments from image
//     - generates mesh from these points given an interior point for each
//     compartment
//     - outputs vertices and triangles in GMSH .msh format

#pragma once

#include <QImage>
#include <QPointF>
#include <QRgb>
#include <QString>
#include <QSize>
#include <array>
#include <cstddef>
#include <memory>
#include <string>
#include <unordered_set>
#include <utility>
#include <vector>

namespace mesh {

using QTriangleF = std::array<QPointF, 3>;
using TriangulateTriangleIndex = std::array<std::size_t, 3>;

class Boundary;

class Mesh {
private:
  bool readOnlyMesh{false};
  bool validMesh{true};
  // input data
  QImage img;
  QPointF origin;
  double pixel{};
  std::vector<std::vector<QPointF>> compartmentInteriorPoints;
  std::vector<std::size_t> boundaryMaxPoints;
  std::vector<std::size_t> compartmentMaxTriangleArea;
  // generated data
  std::unique_ptr<std::vector<Boundary>> boundaries;
  std::vector<QPointF> vertices;
  std::size_t nTriangles{};
  std::vector<std::vector<QTriangleF>> triangles;
  std::vector<std::vector<TriangulateTriangleIndex>> triangleIndices;
  // convert point in pixel units to point in physical units
  QPointF pixelPointToPhysicalPoint(const QPointF &pixelPoint) const noexcept;
  void constructMesh();

public:
  Mesh();
  // constructor to generate mesh from supplied image
  explicit Mesh(const QImage &image,
                const std::vector<std::vector<QPointF>> &interiorPoints = {},
                std::vector<std::size_t> maxPoints = {},
                std::vector<std::size_t> maxTriangleArea = {},
                double pixelWidth = 1.0,
                const QPointF &originPoint = QPointF(0, 0),
                const std::vector<QRgb> &compartmentColours = {});
  // constructor to load existing vertices & triangles without original image
  Mesh(const std::vector<double> &inputVertices,
       const std::vector<std::vector<int>> &inputTriangleIndices,
       const std::vector<std::vector<QPointF>> &interiorPoints);
  ~Mesh();
  // if mesh not constructed from image, but supplied as vertices&trianges,
  // we cannot alter boundary or triangle area easily, so treat as read-only
  bool isReadOnly() const;
  bool isValid() const;
  std::size_t getNumBoundaries() const;
  void setBoundaryMaxPoints(std::size_t boundaryIndex, std::size_t maxPoints);
  std::size_t getBoundaryMaxPoints(std::size_t boundaryIndex) const;
  std::vector<std::size_t> getBoundaryMaxPoints() const;
  void setCompartmentMaxTriangleArea(std::size_t compartmentIndex,
                                     std::size_t maxTriangleArea);
  std::size_t getCompartmentMaxTriangleArea(std::size_t compartmentIndex) const;
  const std::vector<std::size_t> &getCompartmentMaxTriangleArea() const;
  void setPhysicalGeometry(double pixelWidth,
                           const QPointF &originPoint = QPointF(0, 0));
  // return vertices as an array of doubles for SBML
  std::vector<double> getVerticesAsFlatArray() const;
  // return triangle indices as an array of ints for SBML
  std::vector<int>
  getTriangleIndicesAsFlatArray(std::size_t compartmentIndex) const;
  const std::vector<std::vector<TriangulateTriangleIndex>> &getTriangleIndices() const;
  const std::vector<std::vector<QTriangleF>> &getTriangles() const;

  std::pair<QImage, QImage>
  getBoundariesImages(const QSize &size, std::size_t boldBoundaryIndex) const;
  std::pair<QImage, QImage> getMeshImages(const QSize &size,
                                          std::size_t compartmentIndex) const;
  QString getGMSH(const std::unordered_set<int> &gmshCompIndices = {}) const;
};

} // namespace mesh
