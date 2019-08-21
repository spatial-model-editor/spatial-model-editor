#include "boundary.hpp"

#include <set>

#include "logger.hpp"
/*
static int qPointToInt(const QPoint& point) {
  return point.x() + 65536 * point.y();
}

static int pointToIndex(const QPoint& p) { return p.x() + img.width() * p.y(); }

static inline QPoint indexToPoint(int i) {
  return QPoint(i % img.width(), i / img.width());
}
*/
namespace boundary {

bool BoundaryBoolGrid::isBoundary(std::size_t x, std::size_t y) const {
  return grid[x + 1][y + 1];
}

bool BoundaryBoolGrid::isBoundary(const QPoint& point) const {
  auto x = static_cast<std::size_t>(point.x());
  auto y = static_cast<std::size_t>(point.y());
  return isBoundary(x, y);
}

bool BoundaryBoolGrid::isFixed(std::size_t x, std::size_t y) const {
  return fixedPointIndex[x + 1][y + 1] != NULL_INDEX;
}

bool BoundaryBoolGrid::isFixed(const QPoint& point) const {
  auto x = static_cast<std::size_t>(point.x());
  auto y = static_cast<std::size_t>(point.y());
  return isFixed(x, y);
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
  return fixedPoints.at(getFixedPointIndex(x, y));
}

void BoundaryBoolGrid::setBoundaryPoint(const QPoint& point, bool multi) {
  auto x = static_cast<std::size_t>(point.x() + 1);
  auto y = static_cast<std::size_t>(point.y() + 1);
  if (multi) {
    std::size_t i;
    if (isFixed(point)) {
      i = getFixedPointIndex(point);
      spdlog::debug(
          "BoundaryBoolGrid::setBoundaryPoint :: adding {} to existing FP {}",
          point, fixedPoints.at(i));
    } else {
      spdlog::debug("BoundaryBoolGrid::setBoundaryPoint :: creating new FP {}",
                    point);
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
  spdlog::debug("BoundaryBoolGrid::setBoundaryPoint :: BP: {}", point);
  grid[x][y] = true;
}

void BoundaryBoolGrid::visitPoint(std::size_t x, std::size_t y) {
  grid[x + 1][y + 1] = false;
}

void BoundaryBoolGrid::visitPoint(const QPoint& point) {
  auto x = static_cast<std::size_t>(point.x());
  auto y = static_cast<std::size_t>(point.y());
  visitPoint(x, y);
}

BoundaryBoolGrid::BoundaryBoolGrid(const QImage& inputImage)
    : grid(static_cast<size_t>(inputImage.width() + 2),
           std::vector<bool>(static_cast<size_t>(inputImage.height() + 2),
                             false)),
      fixedPointIndex(
          static_cast<size_t>(inputImage.width() + 2),
          std::vector<std::size_t>(static_cast<size_t>(inputImage.height() + 2),
                                   NULL_INDEX)) {
  auto img = inputImage.convertToFormat(QImage::Format_Indexed8);
  spdlog::debug("BoundaryBoolGrid::BoundaryBoolGrid :: nColours = {}",
                img.colorCount());
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
          colours.insert(img.pixelIndex(nn));
        } else {
          // assume external pixels have same colour index as top-left pixel
          colours.insert(img.pixelIndex(0, 0));
        }
      }
      int largestColIndex = *colours.rbegin();
      std::size_t neighbours = colours.size() - 1;
      // QImage has (0,0) in top left
      // we want to generate mesh with (0,0) in bottom left
      // so here we invert the y value
      QPoint bottomLeftIndexedPoint = QPoint(x, img.height() - 1 - y);
      if (neighbours == 1 && largestColIndex == colIndex) {
        // - point has one other colour as neighbour
        // - this colour index is the larger
        setBoundaryPoint(bottomLeftIndexedPoint);
      } else if (neighbours > 1) {
        // - point has multiple colour neighbours
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

Boundary::Boundary(const std::vector<QPoint>& boundaryPoints, bool isClosedLoop)
    : vertices(boundaryPoints), isLoop(isClosedLoop) {
  // remove degenerate points from vertices
  removeDegenerateVertices();

  // construct list of points in reverse order of importance
  // start by using all points
  std::vector<std::size_t> pointIndices;
  pointIndices.reserve(vertices.size());
  for (std::size_t i = 0; i < vertices.size(); ++i) {
    pointIndices.push_back(i);
  }
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
    }
  }
}

std::vector<Boundary> constructBoundaries(const QImage& img) {
  std::vector<Boundary> boundaries;
  std::vector<QPoint> nearestNeighbourDirectionPoints = {
      QPoint(1, 0), QPoint(-1, 0), QPoint(0, 1),  QPoint(0, -1),
      QPoint(1, 1), QPoint(1, -1), QPoint(-1, 1), QPoint(-1, -1)};

  // add image boundary loop
  // todo: decide if this should be done or not
  std::vector<QPoint> points;
  points.push_back(QPoint(0, 0));
  points.push_back(QPoint(0, img.height() - 1));
  points.push_back(QPoint(img.width() - 1, img.height() - 1));
  points.push_back(QPoint(img.width() - 1, 0));
  boundaries.emplace_back(points, true);

  // construct bool grid of all boundary points
  BoundaryBoolGrid bbg(img);

  // we now have an unordered set of all boundary points
  // with points used by multiple boundary lines identified as "fixed"
  //
  // two possible kinds of boundary
  //   - line between two fixed point
  //   - closed loop not involving any fixed points

  // do line between fixed points first:
  //   - start at a fixed point
  //   - visit nearest (unvisited) neighbouring
  //   boundary point in x or y
  //   - if not found, check diagonal neighbours
  //   - repeat until we hit another fixed point
  for (std::size_t i = 0; i < bbg.fixedPoints.size(); ++i) {
    const auto& fp = bbg.fixedPoints[i];
    bbg.visitPoint(fp);
    for (const auto& dfp : nearestNeighbourDirectionPoints) {
      if (bbg.isBoundary(fp + dfp) && bbg.fixedPointCounter.at(i) > 0) {
        points.clear();
        points.push_back(fp);
        QPoint currPoint = fp + dfp;
        while (!bbg.isFixed(currPoint) ||
               (bbg.getFixedPoint(currPoint) == fp)) {
          // repeat until we hit another fixed point
          if (!(bbg.isFixed(currPoint) &&
                (bbg.getFixedPoint(currPoint) == fp))) {
            // only add point to boundary if
            // we are outside of the initial fixed point radius
            points.push_back(currPoint);
          }
          bbg.visitPoint(currPoint);
          for (const auto& directionPoint : nearestNeighbourDirectionPoints) {
            if (bbg.isBoundary(currPoint + directionPoint)) {
              currPoint += directionPoint;
              break;
            }
          }
        }
        // add final fixed point to boundary line
        points.push_back(bbg.getFixedPoint(currPoint));
        // we now have a line between two fixed points
        boundaries.emplace_back(points, false);
        --bbg.fixedPointCounter[i];
        --bbg.fixedPointCounter[bbg.getFixedPointIndex(currPoint)];
      }
    }
  }
  // any remaining points should be independent closed loops:
  //   - find (any non-fixed) boundary point
  //   - visit nearest (unvisited) neighbouring boundary point in x or y
  //   - if not found, check diagonal neighbours
  //   - if not found, loop is done
  bool foundStartPoint;
  do {
    // find a boundary point to start from
    QPoint startPoint;
    foundStartPoint = false;
    for (int x = 0; x < img.width(); ++x) {
      for (int y = 0; y < img.height(); ++y) {
        startPoint = QPoint(x, y);
        if (bbg.isBoundary(startPoint) && !bbg.isFixed(startPoint)) {
          foundStartPoint = true;
          break;
        }
      }
      if (foundStartPoint) {
        break;
      }
    }
    if (foundStartPoint) {
      points.clear();
      QPoint currPoint = startPoint;
      bool finished = false;
      while (!finished) {
        points.push_back(currPoint);
        finished = true;
        bbg.visitPoint(currPoint);
        for (const auto& directionPoint : nearestNeighbourDirectionPoints) {
          if (bbg.isBoundary(currPoint + directionPoint) &&
              !bbg.isFixed(currPoint + directionPoint)) {
            currPoint += directionPoint;
            finished = false;
            break;
          }
        }
      }
      boundaries.emplace_back(points, true);
    }
  } while (foundStartPoint);

  return boundaries;
}

}  // namespace boundary
