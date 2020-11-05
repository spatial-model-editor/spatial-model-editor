#include "image_boundaries.hpp"
#include "boundary.hpp"
#include "contours.hpp"
#include "logger.hpp"
#include <QPoint>
#include <algorithm>
#include <cmath>
#include <limits>
#include <memory>

namespace mesh {

ImageBoundaries::ImageBoundaries() = default;

ImageBoundaries::ImageBoundaries(
    const QImage &img, const std::vector<QRgb> &compartmentColours,
    const std::vector<std::pair<std::string, ColourPair>>
        &membraneColourPairs) {
  auto contours = Contours(img, compartmentColours, membraneColourPairs);
  fixedPoints = std::move(contours.getFixedPoints());
  boundaries = std::move(contours.getBoundaries());
  boundaryPixelsImage = contours.getBoundaryPixelsImage();
}

ImageBoundaries::~ImageBoundaries() = default;

const std::vector<Boundary> &ImageBoundaries::getBoundaries() const {
  return boundaries;
}

const std::vector<FixedPoint> &ImageBoundaries::getFixedPoints() const {
  return fixedPoints;
}

const QImage &ImageBoundaries::getBoundaryPixelsImage() const {
  return boundaryPixelsImage;
}

void ImageBoundaries::setMaxPoints(
    const std::vector<std::size_t> &boundaryMaxPoints) {
  for (std::size_t i = 0; i < boundaries.size(); ++i) {
    boundaries[i].setMaxPoints(boundaryMaxPoints[i]);
  }
}

std::vector<std::size_t> ImageBoundaries::setAutoMaxPoints() {
  std::vector<std::size_t> maxPoints;
  maxPoints.reserve(boundaries.size());
  for (auto &b : boundaries) {
    maxPoints.push_back(b.setMaxPoints());
  }
  return maxPoints;
}

void ImageBoundaries::setMaxPoints(std::size_t boundaryIndex,
                                   std::size_t maxPoints) {
  boundaries[boundaryIndex].setMaxPoints(maxPoints);
}

} // namespace mesh
