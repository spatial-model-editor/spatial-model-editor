// BoundaryBoolGrid
//   - inputs:
//      - segmented image of geometry: one colour for each compartment
//      - list of compartment colours
//      - list of membrane colour pairs
//   - determines if each point is
//      - a boundary pixel
//      - a membrane boundary pixel between two compartments
//      - a fixed point where 3 compartments meet

#pragma once

#include <QImage>
#include <optional>
#include <string>
#include <vector>

namespace mesh {

using ColourPair = std::pair<QRgb, QRgb>;

class BoundaryPixels {
private:
  QImage boundaryPixelsImage;
  std::vector<std::size_t> values;
  std::size_t nCompartments;
  std::size_t membraneIndexOffset;
  std::size_t fpIndexOffset;
  std::vector<std::size_t> membraneIndices;
  std::vector<QRgb> pixelColours;
  std::vector<std::string> membraneNames;
  std::string defaultMembraneName = {};
  std::vector<QPoint> fixedPoints;
  inline std::size_t toIndex(const QPoint &point) const noexcept;
  void setBoundaryPoint(const QPoint &point, std::size_t value);
  void setFixedPoint(const QPoint &point);
  void setPointColour(const QPoint &point, std::size_t value);
  std::size_t getOrInsertFixedPointIndex(const QPoint &point);

public:
  int width() const;
  int height() const;
  bool isValid(const QPoint &point) const;
  bool isBoundary(const QPoint &point) const;
  bool isFixed(const QPoint &point) const;
  bool isMembrane(const QPoint &point) const;
  std::size_t getMembraneIndex(const QPoint &point) const;
  const std::string &getMembraneName(std::size_t i) const;
  std::size_t getFixedPointIndex(const QPoint &point) const;
  const QPoint &getFixedPoint(const QPoint &point) const;
  std::optional<QPoint> getNeighbourOnBoundary(const QPoint &point) const;
  const std::vector<QPoint> &getFixedPoints() const;
  const QImage &getBoundaryPixelsImage() const;
  void visitPoint(const QPoint &point);
  explicit BoundaryPixels(const QImage &inputImage,
                          const std::vector<QRgb> &compartmentColours = {},
                          const std::vector<std::pair<std::string, ColourPair>>
                              &membraneColourPairs = {});
};

} // namespace mesh
