#include "boundary.hpp"

#include <array>
#include <optional>
#include <set>

#include "logger.hpp"

namespace boundary {

bool BoundaryBoolGrid::isBoundary(std::size_t x, std::size_t y) const {
  return grid[x + 1][y + 1];
}

const QSize& BoundaryBoolGrid::size() const { return imageSize; }

bool BoundaryBoolGrid::isValid(const QPoint& point) const {
  auto x = static_cast<std::size_t>(point.x());
  auto y = static_cast<std::size_t>(point.y());
  auto w = static_cast<std::size_t>(imageSize.width());
  auto h = static_cast<std::size_t>(imageSize.height());
  return x < w && y < h;
}

bool BoundaryBoolGrid::isBoundary(const QPoint& point) const {
  auto x = static_cast<std::size_t>(point.x());
  auto y = static_cast<std::size_t>(point.y());
  return isBoundary(x, y);
}

bool BoundaryBoolGrid::isFixed(const QPoint& point) const {
  return getFixedPointIndex(point) != NULL_INDEX;
}

bool BoundaryBoolGrid::isMembrane(const QPoint& point) const {
  return getMembraneIndex(point) != NULL_INDEX;
}

std::size_t BoundaryBoolGrid::getMembraneIndex(const QPoint& point) const {
  auto x = static_cast<std::size_t>(point.x());
  auto y = static_cast<std::size_t>(point.y());
  return membraneIndex[x + 1][y + 1];
}

std::string BoundaryBoolGrid::getMembraneName(std::size_t i) const {
  auto iter = membraneNames.find(i);
  if (iter != membraneNames.cend()) {
    return iter->second;
  }
  return {};
}

std::size_t BoundaryBoolGrid::getFixedPointIndex(std::size_t x,
                                                 std::size_t y) const {
  return fixedPointIndex[x + 1][y + 1];
}

std::size_t BoundaryBoolGrid::getFixedPointIndex(const QPoint& point) const {
  auto x = static_cast<std::size_t>(point.x());
  auto y = static_cast<std::size_t>(point.y());
  return getFixedPointIndex(x, y);
}

const QPoint& BoundaryBoolGrid::getFixedPoint(const QPoint& point) const {
  auto x = static_cast<std::size_t>(point.x());
  auto y = static_cast<std::size_t>(point.y());
  return fixedPoints[getFixedPointIndex(x, y)];
}

void BoundaryBoolGrid::setBoundaryPoint(const QPoint& point, bool multi,
                                        std::size_t iMembrane) {
  auto x = static_cast<std::size_t>(point.x() + 1);
  auto y = static_cast<std::size_t>(point.y() + 1);
  if (multi) {
    std::size_t i;
    if (isFixed(point)) {
      i = getFixedPointIndex(point);
    } else {
      fixedPoints.push_back(point);
      fixedPointCounter.push_back(3);
      i = fixedPoints.size() - 1;
    }
    // set all neighbouring pixels to point to this fixed point
    fixedPointIndex[x][y] = i;
    fixedPointIndex[x + 1][y] = i;
    fixedPointIndex[x - 1][y] = i;
    fixedPointIndex[x][y + 1] = i;
    fixedPointIndex[x][y - 1] = i;
    fixedPointIndex[x + 1][y + 1] = i;
    fixedPointIndex[x + 1][y - 1] = i;
    fixedPointIndex[x - 1][y + 1] = i;
    fixedPointIndex[x - 1][y - 1] = i;
  }
  grid[x][y] = true;
  membraneIndex[x][y] = iMembrane;
}

void BoundaryBoolGrid::visitPoint(std::size_t x, std::size_t y) {
  grid[x + 1][y + 1] = false;
}

void BoundaryBoolGrid::visitPoint(const QPoint& point) {
  auto x = static_cast<std::size_t>(point.x());
  auto y = static_cast<std::size_t>(point.y());
  visitPoint(x, y);
}

QImage BoundaryBoolGrid::getBoundaryPixelsImage() const {
  QImage img(imageSize, QImage::Format_ARGB32_Premultiplied);
  img.fill(0);
  for (int x = 0; x < imageSize.width(); ++x) {
    for (int y = 0; y < imageSize.height(); ++y) {
      QPoint p(x, y);
      QPoint pInv(x, imageSize.height() - y - 1);
      if (isBoundary(p)) {
        img.setPixel(pInv, 0xff888888);
      }
      if (isMembrane(p)) {
        img.setPixel(pInv, 0xffff0000);
      }
      if (isFixed(p)) {
        img.setPixel(pInv, 0xff0000ff);
      }
    }
  }
  return img;
}

BoundaryBoolGrid::BoundaryBoolGrid(
    const QImage& inputImage,
    const std::map<ColourPair, std::pair<std::size_t, std::string>>&
        mapColourPairToMembraneIndex,
    const std::vector<QRgb>& compartmentColours)
    : grid(static_cast<size_t>(inputImage.width() + 2),
           std::vector<bool>(static_cast<size_t>(inputImage.height() + 2),
                             false)),
      fixedPointIndex(
          static_cast<size_t>(inputImage.width() + 2),
          std::vector<std::size_t>(static_cast<size_t>(inputImage.height() + 2),
                                   NULL_INDEX)),
      imageSize(inputImage.size()) {
  membraneIndex = fixedPointIndex;
  membraneNames.clear();
  auto img = inputImage.convertToFormat(QImage::Format_Indexed8);
  std::vector<int> compartmentColourIndices;
  for (int i = 0; i < img.colorCount(); ++i) {
    if (std::find(compartmentColours.cbegin(), compartmentColours.cend(),
                  img.color(i)) != compartmentColours.cend()) {
      compartmentColourIndices.push_back(i);
    }
  }
  nPixels = static_cast<std::size_t>(img.width() * img.height());
  // check colours of 4 nearest neighbours of each pixel
  for (int x = 0; x < img.width(); ++x) {
    for (int y = 0; y < img.height(); ++y) {
      QPoint point(x, y);
      int colIndex = img.pixelIndex(point);
      std::set<int> colours;
      colours.insert(colIndex);
      for (const auto& dp :
           {QPoint(1, 0), QPoint(-1, 0), QPoint(0, 1), QPoint(0, -1)}) {
        QPoint nn = point + dp;
        if (img.valid(nn)) {
          int idx = img.pixelIndex(nn);
          colours.insert(idx);
        } else {
          // external pixels
          colours.insert(-1);
        }
      }
      int largestColIndex = *colours.rbegin();
      std::size_t neighbours = colours.size() - 1;
      // QImage has (0,0) in top left
      // we want to generate mesh with (0,0) in bottom left
      // so here we invert the y value
      QPoint bottomLeftIndexedPoint = QPoint(x, img.height() - 1 - y);
      if (neighbours == 1 && largestColIndex == colIndex) {
        QRgb colA = img.color(*colours.rbegin());
        QRgb colB = 0;
        if (int ci = *colours.begin(); ci >= 0) {
          colB = img.color(ci);
        }
        // - point has one other colour as neighbour
        // - this colour index is the larger
        if (auto iter = mapColourPairToMembraneIndex.find({colA, colB});
            iter != mapColourPairToMembraneIndex.cend()) {
          // - these two colours correspond to a membrane
          std::size_t index = iter->second.first;
          setBoundaryPoint(bottomLeftIndexedPoint, false, index);
          if (membraneNames.find(index) == membraneNames.cend()) {
            membraneNames[index] = iter->second.second;
          }
        } else if (std::find_if(compartmentColourIndices.cbegin(),
                                compartmentColourIndices.cend(),
                                [&colours](int i) {
                                  return colours.find(i) != colours.cend();
                                }) != compartmentColourIndices.cend()) {
          // at least one colour corresponds to a compartment
          setBoundaryPoint(bottomLeftIndexedPoint, false);
        }
      } else if (neighbours > 1) {
        // - point has multiple colour neighbours -> fixed point
        setBoundaryPoint(bottomLeftIndexedPoint, true);
      }
    }
  }
}

static double triangleArea(const QPoint& a, const QPoint& b, const QPoint& c) {
  // https://en.wikipedia.org/wiki/Shoelace_formula
  return 0.5 * std::fabs(a.x() * b.y() + b.x() * c.y() + c.x() * a.y() -
                         b.x() * a.y() - c.x() * b.y() - a.x() * c.y());
}

void Boundary::removeDegenerateVertices() {
  std::size_t size = vertices.size();
  std::size_t iMin = 0;
  std::size_t iMax = size;
  auto iter = vertices.begin();
  // if boundary is not a loop, then first and last points are fixed
  if (!isLoop) {
    ++iMin;
    ++iter;
    --iMax;
  }
  // remove any points that have a zero area triangle
  for (std::size_t i = iMin; i < iMax; ++i) {
    std::size_t ip = (i + 1) % size;
    std::size_t im = (i + size - 1) % size;
    double area = triangleArea(vertices[im], vertices[i], vertices[ip]);
    if (area < 1e-14) {
      vertices.erase(iter);
      removeDegenerateVertices();
      break;
    }
    ++iter;
  }
  return;
}

void Boundary::constructNormalUnitVectors() {
  normalUnitVectors.clear();
  if (!isLoop) {
    // for now we only support for closed loops
    return;
  }
  normalUnitVectors.reserve(vertices.size());
  // calculate unit vector normal (perpendicular) to boundary
  for (std::size_t i = 0; i < vertices.size(); ++i) {
    std::size_t ip = (i + 1) % vertices.size();
    std::size_t im = (i + vertices.size() - 1) % vertices.size();
    QPoint delta = vertices[ip] - vertices[im];
    auto normal = QPointF(-delta.y(), delta.x());
    normal /= sqrt(QPointF::dotProduct(normal, normal));
    normalUnitVectors.push_back(normal);
  }
}

// Visvalingam polyline simplification
// return index of vertex which forms smallest area triangle
// NB: very inefficient & not quite right:
// when recalculating triangle, if new one is smaller
// than the one just removed, should use the just-removed value
// for the area - but good enough for now & also works for closed loops
std::vector<std::size_t>::const_iterator Boundary::smallestTrianglePointIndex(
    const std::vector<std::size_t>& pointIndices) const {
  std::vector<std::size_t>::const_iterator iterSmallest;
  auto iter = pointIndices.cbegin();
  std::size_t size = pointIndices.size();
  double minArea = std::numeric_limits<double>::max();
  std::size_t iMin = 0;
  std::size_t iMax = size;
  // if boundary is not a loop, then first and last points are fixed
  if (!isLoop) {
    ++iMin;
    ++iter;
    --iMax;
  }
  // calculate area of triangle for each point
  for (std::size_t i = iMin; i < iMax; ++i) {
    std::size_t ip = (i + 1) % size;
    std::size_t im = (i + size - 1) % size;
    double area =
        triangleArea(vertices[pointIndices[im]], vertices[pointIndices[i]],
                     vertices[pointIndices[ip]]);
    if (area < minArea) {
      minArea = area;
      iterSmallest = iter;
    }
    ++iter;
  }
  // return iterator to index of point with smallest triangle
  return iterSmallest;
}

Boundary::Boundary(const std::vector<QPoint>& boundaryPoints, bool isClosedLoop,
                   bool isMembraneCompartment, const std::string& membraneName)
    : vertices(boundaryPoints),
      isLoop(isClosedLoop),
      isMembrane(isMembraneCompartment),
      membraneID(membraneName) {
  removeDegenerateVertices();
  constructNormalUnitVectors();

  // construct list of points in reverse order of importance
  orderedBoundaryIndices.reserve(vertices.size());
  // start by using all points
  std::vector<std::size_t> pointIndices(vertices.size());
  std::iota(pointIndices.begin(), pointIndices.end(), 0);
  // remove one-by-one to get point indices in reverse order of importance
  while (pointIndices.size() > 3) {
    auto iter = smallestTrianglePointIndex(pointIndices);
    orderedBoundaryIndices.push_back(*iter);
    pointIndices.erase(iter);
  }
  // add last 3 points
  for (auto i : pointIndices) {
    orderedBoundaryIndices.push_back(i);
  }

  // set initial boundary to use all possible points
  setMaxPoints(vertices.size());
}

std::size_t Boundary::getMaxPoints() const { return maxPoints; }

void Boundary::setMaxPoints(std::size_t nMaxPoints) {
  maxPoints = nMaxPoints;
  points.clear();
  points.reserve(std::min(maxPoints, vertices.size()));
  outerPoints.clear();
  outerPoints.reserve(std::min(maxPoints, vertices.size()));
  std::vector<bool> usePoint(vertices.size(), false);
  auto iter = orderedBoundaryIndices.crbegin();
  while (iter != orderedBoundaryIndices.crend() && nMaxPoints != 0) {
    usePoint.at(*iter) = true;
    ++iter;
    --nMaxPoints;
  }
  for (std::size_t i = 0; i < vertices.size(); ++i) {
    if (usePoint.at(i)) {
      points.push_back(vertices[i]);
      if (isMembrane && isLoop) {
        // note: non-loop membranes not yet supported
        outerPoints.push_back(vertices[i] +
                              membraneWidth * normalUnitVectors[i]);
      }
    }
  }
}

double Boundary::getMembraneWidth() const { return membraneWidth; }

void Boundary::setMembraneWidth(double newMembraneWidth) {
  membraneWidth = newMembraneWidth;
  setMaxPoints(maxPoints);
}

static std::vector<QPoint> getBPNeighboursOfFP(const QPoint& fp,
                                               const BoundaryBoolGrid& bbg) {
  std::vector<QPoint> neighbours;
  neighbours.reserve(8);
  constexpr std::array<QPoint, 4> nnp = {QPoint(1, 0), QPoint(-1, 0),
                                         QPoint(0, 1), QPoint(0, -1)};
  SPDLOG_TRACE("FP ({},{})", fp.x(), fp.y());
  // fill queue with all pixels in this FP
  std::vector<QPoint> queue;
  queue.reserve(8);
  queue.push_back(fp);
  std::size_t queueIndex = 0;
  while (queueIndex < queue.size()) {
    QPoint p = queue[queueIndex];
    for (const auto& dp : nnp) {
      if (QPoint bp = p + dp;
          bbg.isValid(bp) && bbg.isFixed(bp) &&
          std::find(queue.cbegin(), queue.cend(), bp) == queue.cend()) {
        queue.push_back(bp);
      }
    }
    ++queueIndex;
  }
  // return all non-FP BP neighbours of this fixed point
  for (const auto& p : queue) {
    for (const auto& dp : nnp) {
      if (QPoint bp = p + dp;
          bbg.isValid(bp) && !bbg.isFixed(bp) && bbg.isBoundary(bp)) {
        neighbours.push_back(bp);
        SPDLOG_TRACE("  - ({},{})", bp.x(), bp.y());
      }
    }
  }
  return neighbours;
}

static std::optional<QPoint> getBPNeighbourOfBP(const QPoint& bp,
                                                const BoundaryBoolGrid& bbg) {
  constexpr std::array<QPoint, 8> nnp = {
      QPoint(1, 0), QPoint(-1, 0), QPoint(0, 1),  QPoint(0, -1),
      QPoint(1, 1), QPoint(1, -1), QPoint(-1, 1), QPoint(-1, -1)};
  for (const auto& dp : nnp) {
    if (QPoint n = bp + dp;
        bbg.isValid(n) && bbg.isBoundary(n) && !bbg.isFixed(n)) {
      return n;
    }
  }
  return {};
}

static std::optional<QPoint> getFPNeighbourOfBP(const QPoint& bp,
                                                const BoundaryBoolGrid& bbg) {
  constexpr std::array<QPoint, 8> nnp = {
      QPoint(1, 0), QPoint(-1, 0), QPoint(0, 1),  QPoint(0, -1),
      QPoint(1, 1), QPoint(1, -1), QPoint(-1, 1), QPoint(-1, -1)};
  for (const auto& dp : nnp) {
    if (QPoint n = bp + dp;
        bbg.isValid(n) && bbg.isBoundary(n) && bbg.isFixed(n)) {
      return bbg.getFixedPoint(n);
    }
  }
  return {};
}

static std::optional<QPoint> getBoundaryPoint(const BoundaryBoolGrid& bbg) {
  for (int x = 0; x < bbg.size().width(); ++x) {
    for (int y = 0; y < bbg.size().height(); ++y) {
      auto p = QPoint(x, y);
      if (bbg.isBoundary(p) && !bbg.isFixed(p)) {
        return p;
      }
    }
  }
  return {};
}

std::pair<std::vector<Boundary>, QImage> constructBoundaries(
    const QImage& img,
    const std::map<ColourPair, std::pair<std::size_t, std::string>>&
        mapColourPairToMembraneIndex,
    const std::vector<QRgb>& compartmentColours) {
  std::vector<Boundary> boundaries;

  // construct bool grid of all boundary points
  BoundaryBoolGrid bbg(img, mapColourPairToMembraneIndex, compartmentColours);
  QImage boundaryPixelsImage = bbg.getBoundaryPixelsImage();

  // we now have an unordered set of all boundary points
  // with points used by multiple boundary lines identified as "fixed"
  //
  // two possible kinds of boundary
  //   - line between two fixed points
  //   - closed loop not involving any fixed points

  // do lines between fixed points first:
  //   - start at a fixed point
  //   - visit nearest (unvisited) neighbouring
  //   boundary point in x or y
  //   - if not found, check diagonal neighbours
  //   - repeat until we hit another fixed point
  for (const auto& fp : bbg.fixedPoints) {
    SPDLOG_TRACE("fixedPoint ({},{})", fp.x(), fp.y());
    for (const auto& startPoint : getBPNeighboursOfFP(fp, bbg)) {
      std::vector<QPoint> points{fp};
      bbg.visitPoint(startPoint);
      SPDLOG_TRACE("  - ({},{})", startPoint.x(), startPoint.y());
      points.push_back(startPoint);
      // visit boundary neighbours until we can't find another non-fp
      // neighbour
      auto currPoint = getBPNeighbourOfBP(startPoint, bbg);
      while (currPoint.has_value()) {
        bbg.visitPoint(currPoint.value());
        SPDLOG_TRACE("  - ({},{})", currPoint.value().x(),
                     currPoint.value().y());
        points.push_back(currPoint.value());
        currPoint = getBPNeighbourOfBP(currPoint.value(), bbg);
      }
      // look for neighbour of last point which is in a FP
      if (auto lastPoint = getFPNeighbourOfBP(points.back(), bbg);
          lastPoint.has_value()) {
        // add final fixed point to boundary line
        points.push_back(lastPoint.value());
        SPDLOG_TRACE("  -> fixedPoint ({},{})", points.back().x(),
                     points.back().y());
        SPDLOG_TRACE("  [{} connecting pixels]", points.size());
        // check if is membrane - for now just check a point in the middle
        bool isMembrane = false;
        std::string membraneName;
        if (auto p = points[points.size() / 2]; bbg.isMembrane(p)) {
          isMembrane = true;
          membraneName = bbg.getMembraneName(bbg.getMembraneIndex(p));
        }
        SPDLOG_TRACE("  membrane={}", membraneName);
        // add line to boundaries
        boundaries.emplace_back(points, false, isMembrane, membraneName);
      } else {
        SPDLOG_WARN(
            "Failed to find FP neighbour of point ({},{}): skipping this "
            "boundary line",
            points.back().x(), points.back().y());
      }
    }
  }

  // any remaining points should be independent closed loops:
  //   - find (any non-fixed) boundary point
  //   - visit nearest (unvisited) neighbouring boundary point in x or y
  //   - if not found, check diagonal neighbours
  //   - if not found, loop is done
  auto startPoint = getBoundaryPoint(bbg);
  while (startPoint.has_value()) {
    SPDLOG_TRACE("loop start point ({},{})", startPoint.value().x(),
                 startPoint.value().y());
    bool isMembrane = bbg.isMembrane(startPoint.value());
    std::size_t membraneIndex = bbg.getMembraneIndex(startPoint.value());
    std::vector<QPoint> points{startPoint.value()};
    bbg.visitPoint(startPoint.value());
    auto currPoint = getBPNeighbourOfBP(startPoint.value(), bbg);
    while (currPoint.has_value()) {
      points.push_back(currPoint.value());
      bbg.visitPoint(currPoint.value());
      if (isMembrane &&
          bbg.getMembraneIndex(currPoint.value()) != membraneIndex) {
        SPDLOG_WARN("inconsistent membrane index: {} and {}", membraneIndex,
                    bbg.getMembraneIndex(currPoint.value()));
      }
      currPoint = getBPNeighbourOfBP(currPoint.value(), bbg);
    }
    SPDLOG_TRACE("  - {} points", points.size());
    SPDLOG_TRACE("  - membrane: {}", isMembrane);
    SPDLOG_TRACE("  - membrane index: {}", membraneIndex);
    SPDLOG_TRACE("  - membrane name: {}", bbg.getMembraneName(membraneIndex));
    boundaries.emplace_back(points, true, isMembrane,
                            bbg.getMembraneName(membraneIndex));
    startPoint = getBoundaryPoint(bbg);
  }

  return {boundaries, boundaryPixelsImage};
}

}  // namespace boundary
