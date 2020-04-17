// Meshing
//  - Mesh class:
//     - generates simplified boundary vertices and segments from image
//     - generates mesh from these points given an interior point for each
//     compartment
//     - outputs vertices and triangles in GMSH .msh format

#pragma once

#include <QImage>
#include <QPoint>
#include <QPointF>
#include <array>
#include <unordered_set>
#include <vector>

#include "boundary.hpp"
#include "image_boundaries.hpp"

namespace mesh {

using QTriangleF = std::array<QPointF, 3>;
using QRectangleF = std::array<QPointF, 4>;
using TriangleIndex = std::array<std::size_t, 3>;
using RectangleIndex = std::array<std::size_t, 4>;
using ColourPair = std::pair<QRgb, QRgb>;

class Mesh {
 private:
  static constexpr std::size_t defaultBoundaryMaxPoints = 12;
  static constexpr std::size_t defaultCompartmentMaxTriangleArea = 40;
  bool readOnlyMesh = false;
  bool validMesh = true;
  // input data
  QImage img;
  QPointF origin;
  double pixel;
  std::vector<QPointF> compartmentInteriorPoints;
  std::vector<std::size_t> boundaryMaxPoints;
  std::vector<std::size_t> compartmentMaxTriangleArea;
  // generated data
  ImageBoundaries imageBoundaries;
  std::vector<QPointF> vertices;
  std::size_t nTriangles;
  std::vector<std::vector<QTriangleF>> triangles;
  std::size_t nRectangles;
  std::vector<std::vector<QRectangleF>> rectangles;
  std::vector<std::vector<TriangleIndex>> triangleIndices;
  std::vector<std::vector<RectangleIndex>> rectangleIndices;
  // convert point in pixel units to point in physical units
  QPointF pixelPointToPhysicalPoint(const QPointF& pixelPoint) const noexcept;
  void constructMesh();

 public:
  Mesh() = default;
  // constructor to generate mesh from supplied image
  explicit Mesh(const QImage& image,
                const std::vector<QPointF>& interiorPoints = {},
                const std::vector<std::size_t>& maxPoints = {},
                const std::vector<std::size_t>& maxTriangleArea = {},
                const std::vector<std::pair<std::string, ColourPair>>&
                    membraneColourPairs = {},
                const std::vector<double>& membraneWidths = {},
                double pixelWidth = 1.0,
                const QPointF& originPoint = QPointF(0, 0),
                const std::vector<QRgb>& compartmentColours = {});
  // constructor to load existing vertices&trianges without original image
  Mesh(const std::vector<double>& inputVertices,
       const std::vector<std::vector<int>>& inputTriangleIndices,
       const std::vector<QPointF>& interiorPoints);
  // if mesh not constructed from image, but supplied as vertices&trianges,
  // we cannot alter boundary or triangle area easily, so treat as read-only
  bool isReadOnly() const;
  bool isValid() const;
  std::size_t getNumBoundaries() const;
  bool isMembrane(std::size_t boundaryIndex) const;
  void setBoundaryMaxPoints(std::size_t boundaryIndex, std::size_t maxPoints);
  std::size_t getBoundaryMaxPoints(std::size_t boundaryIndex) const;
  void setBoundaryWidth(std::size_t boundaryIndex, double width);
  double getBoundaryWidth(std::size_t boundaryIndex) const;
  std::vector<double> getBoundaryWidths() const;
  double getMembraneWidth(const std::string& membraneID) const;
  std::vector<std::size_t> getBoundaryMaxPoints() const;
  void setCompartmentMaxTriangleArea(std::size_t compartmentIndex,
                                     std::size_t maxTriangleArea);
  std::size_t getCompartmentMaxTriangleArea(std::size_t compartmentIndex) const;
  const std::vector<std::size_t>& getCompartmentMaxTriangleArea() const;
  void setPhysicalGeometry(double pixelWidth,
                           const QPointF& originPoint = QPointF(0, 0));
  // return vertices as an array of doubles for SBML
  std::vector<double> getVertices() const;
  // return triangle indices as an array of ints for SBML
  std::vector<int> getTriangleIndices(std::size_t compartmentIndex) const;
  const std::vector<std::vector<QTriangleF>>& getTriangles() const;

  const QImage& getBoundaryPixelsImage() const;
  std::pair<QImage, QImage> getBoundariesImages(
      const QSize& size, std::size_t boldBoundaryIndex) const;
  std::pair<QImage, QImage> getMeshImages(const QSize& size,
                                          std::size_t compartmentIndex) const;
  QString getGMSH(const std::unordered_set<int>& gmshCompIndices = {}) const;
};

}  // namespace mesh
