#include "pixel_corner_iterator.hpp"
#include <algorithm>

namespace sme::mesh {

static std::size_t getFirstCornerIndex(const std::vector<cv::Point> &points,
                                       bool outer) {
  if (points.size() == 1) {
    // single pixel contour: must be outer, and any starting point is valid
    return 1;
  }
  if (outer) {
    // Suzuki algo guarantees that outer contours are oriented anti-clockwise,
    // and that neighbours to the left and above the first pixel are not part
    // of the contour, so top-left corner is always a valid starting point
    return 1;
  }
  // inner contours guaranteed to be oriented clockwise,
  // but we need to choose a valid starting corner depending on the
  // relative orientation of the next pixel in the contour
  auto dp{points[1] - points[0]};
  if (dp.x == 1 && dp.y >= 0) {
    // next pixel E or SE: start at bottom-right corner
    return 3;
  }
  if (dp.y == -1 && dp.x >= 0) {
    // next pixel N or NE: start at top-right corner
    return 0;
  }
  if (dp.x == -1 && dp.y <= 0) {
    // next pixel W or NW: start at top-left corner
    return 1;
  }
  if (dp.y == 1 && dp.x <= 0) {
    // next pixel S or SW: start at bottom-left corner
    return 2;
  }
  // the contour is probably invalid: just return default corner
  return 0;
}

PixelCornerIterator::PixelCornerIterator(const std::vector<cv::Point> &points,
                                         bool outer)
    : pixels(points), cornerIndex(getFirstCornerIndex(points, outer)),
      startVertex(vertex()) {}

static std::size_t next(std::size_t i, std::size_t n) {
  return i + 1 == n ? 0 : i + 1;
}

void PixelCornerIterator::advancePixelIfPossible() {
  auto nextPixelIndex{next(pixelIndex, pixels.size())};
  auto nextCorner{vertex() - pixels[nextPixelIndex]};
  for (std::size_t i = 0; i < corners.size(); ++i) {
    if (nextCorner == corners[i]) {
      cornerIndex = i;
      pixelIndex = nextPixelIndex;
      return;
    }
  }
}

cv::Point PixelCornerIterator::vertex() const {
  return pixels[pixelIndex] + corners[cornerIndex];
}

bool PixelCornerIterator::done() const {
  return vertex() == startVertex && counter > 0;
}

PixelCornerIterator &PixelCornerIterator::operator++() {
  advancePixelIfPossible();
  cornerIndex = next(cornerIndex, corners.size());
  ++counter;
  return *this;
}

} // namespace sme
