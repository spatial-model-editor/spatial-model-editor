// ImageBoundaries

#pragma once

#include "boundary.hpp"
#include "mesh_types.hpp"
#include <QImage>
#include <QPointF>
#include <QRgb>
#include <cstddef>
#include <string>
#include <utility>
#include <vector>

namespace mesh {

class ImageBoundaries {
private:
  std::vector<Boundary> boundaries;
  std::vector<FixedPoint> fixedPoints;
  std::vector<QPointF> newFixedPoints;
  QImage boundaryPixelsImage;

public:
  ImageBoundaries();
  explicit ImageBoundaries(const QImage &img,
                           const std::vector<QRgb> &compartmentColours = {},
                           const std::vector<std::pair<std::string, ColourPair>>
                               &membraneColourPairs = {});
  ~ImageBoundaries();
  const std::vector<Boundary> &getBoundaries() const;
  const std::vector<FixedPoint> &getFixedPoints() const;
  const QImage &getBoundaryPixelsImage() const;
  void setMaxPoints(const std::vector<std::size_t> &boundaryMaxPoints);
  std::vector<std::size_t> setAutoMaxPoints();
  void setMaxPoints(std::size_t boundaryIndex, std::size_t maxPoints = 12);
};

} // namespace mesh
