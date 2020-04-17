#include "image_boundaries.hpp"

#include <array>
#include <optional>
#include <set>

#include "boundary.hpp"
#include "boundary_pixels.hpp"
#include "logger.hpp"

namespace mesh {
static double modAngle(double x) {
  constexpr double pi = 3.14159265358979323846;
  // return angle x as postive number in range [0, 2pi)
  while (x >= 2.0 * pi) {
    x -= 2.0 * pi;
  }
  while (x < 0) {
    x += 2.0 * pi;
  }
  return x;
}

static double getAngleFromLine(const QPointF& p0, const QPointF& p) {
  // get anticlockwise angle of p0->p line relative to horizontal x axis
  return modAngle(std::atan2(p.y() - p0.y(), p.x() - p0.x()));
}

static QPointF makePoint(double radius, double angle, const QPointF& x0) {
  return x0 + QPointF(radius * std::cos(angle), radius * std::sin(angle));
}

static std::vector<QPoint> getBPNeighboursOfFP(const QPoint& fp,
                                               const BoundaryPixels& bbg) {
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
          bbg.isValid(bp) && !bbg.isFixed(bp) && bbg.isBoundary(bp) &&
          std::find(neighbours.cbegin(), neighbours.cend(), bp) ==
              neighbours.cend()) {
        neighbours.push_back(bp);
        SPDLOG_TRACE("  - ({},{})", bp.x(), bp.y());
      }
    }
  }
  return neighbours;
}

static std::optional<QPoint> getFPNeighbourOfBP(const QPoint& bp,
                                                const BoundaryPixels& bbg) {
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

static std::optional<QPoint> getAnyBoundaryPoint(const BoundaryPixels& bbg) {
  for (int x = 0; x < bbg.width(); ++x) {
    for (int y = 0; y < bbg.height(); ++y) {
      if (auto p = QPoint(x, y); bbg.isBoundary(p) && !bbg.isFixed(p)) {
        return p;
      }
    }
  }
  return {};
}

bool ImageBoundaries::fpHasMembrane(const FixedPoint& fp) const {
  return std::any_of(fp.lines.cbegin(), fp.lines.cend(),
                     [](const auto& line) { return line.isMembrane; });
}

double ImageBoundaries::getFpRadius(const FixedPoint& fp) const {
  double radius = std::numeric_limits<double>::max();
  for (const auto& line : fp.lines) {
    if (const auto& b = boundaries[line.boundaryIndex]; b.isMembrane()) {
      radius = std::min(radius, b.getMembraneWidth());
    }
  }
  return radius / std::sqrt(2.0);
}

void ImageBoundaries::updateFpAngles(FixedPoint& fp) {
  for (auto& line : fp.lines) {
    const auto& points = boundaries[line.boundaryIndex].getPoints();
    SPDLOG_TRACE("  - start boundary line [{}]", line.boundaryIndex);
    QPoint p = points[1];
    if (!line.startsFromFP) {
      p = points[points.size() - 2];
    }
    SPDLOG_TRACE("    - nearest point: ({},{})", p.x(), p.y());
    line.angle = getAngleFromLine(fp.point, p);
    SPDLOG_TRACE("    - angle: {}", line.angle);
  }
  std::sort(fp.lines.begin(), fp.lines.end(),
            [](const auto& a, const auto& b) { return a.angle < b.angle; });
}

static double midpointAngle(double a1, double a2) {
  return modAngle(modAngle(a1) + 0.5 * modAngle(a2 - a1));
}

static std::size_t getOrInsertIndex(const QPointF& p,
                                    std::vector<QPointF>& points) {
  // return index of item in points that matches p within error eps
  SPDLOG_TRACE("looking for point ({}, {})", p.x(), p.y());
  constexpr double eps = 1.e-13;
  for (std::size_t i = 0; i < points.size(); ++i) {
    if (QPointF d = points[i] - p; d.manhattanLength() < eps) {
      SPDLOG_TRACE("  -> found [{}] : ({}, {})", i, points[i].x(),
                   points[i].y());
      return i;
    }
  }
  // if not found: add p to vector and return its index
  SPDLOG_TRACE("  -> added new point");
  points.push_back(p);
  return points.size() - 1;
}

void ImageBoundaries::expandFP(FixedPoint& fp) {
  SPDLOG_TRACE("fp: ({},{})", fp.point.x(), fp.point.y());
  double radius = getFpRadius(fp);
  // get angle of each boundary line to the FP
  updateFpAngles(fp);
  for (std::size_t iLine = 0; iLine < fp.lines.size(); ++iLine) {
    const auto& line = fp.lines[iLine];
    const auto& linePlus = fp.lines[(iLine + 1) % fp.lines.size()];
    const auto& lineMinus =
        fp.lines[(iLine + fp.lines.size() - 1) % fp.lines.size()];
    SPDLOG_TRACE("  - start boundary line [{}]", line.boundaryIndex);
    double anglePlus = midpointAngle(line.angle, linePlus.angle);
    double angleMinus = midpointAngle(lineMinus.angle, line.angle);
    if (!line.isMembrane) {
      if (linePlus.isMembrane) {
        angleMinus = anglePlus;
      } else {
        anglePlus = angleMinus;
      }
    }
    SPDLOG_TRACE("    - angle: {}", line.angle);
    SPDLOG_TRACE("    - angle plus: {}", anglePlus);
    SPDLOG_TRACE("    - angle minus: {}", angleMinus);
    auto pointPlus = makePoint(radius, anglePlus, fp.point);
    auto pointMinus = makePoint(radius, angleMinus, fp.point);
    SPDLOG_TRACE("    - point plus: ({},{})", pointPlus.x(), pointPlus.y());
    SPDLOG_TRACE("    - point minus: ({},{})", pointMinus.x(), pointMinus.y());
    auto& b = boundaries[line.boundaryIndex];
    if (line.startsFromFP) {
      if (line.isMembrane) {
        b.setOuterStartPoint(pointPlus,
                             getOrInsertIndex(pointPlus, newFixedPoints));
      }
      b.setInnerStartPoint(pointMinus,
                           getOrInsertIndex(pointMinus, newFixedPoints));
    } else {
      if (line.isMembrane) {
        b.setOuterEndPoint(pointMinus,
                           getOrInsertIndex(pointMinus, newFixedPoints));
      }
      b.setInnerEndPoint(pointPlus,
                         getOrInsertIndex(pointPlus, newFixedPoints));
    }
  }
}

void ImageBoundaries::expandFPs() {
  newFixedPoints.clear();
  newFixedPoints.reserve(fixedPoints.size() * 3);
  for (auto& fp : fixedPoints) {
    if (fpHasMembrane(fp)) {
      expandFP(fp);
    }
  }
}

ImageBoundaries::ImageBoundaries(
    const QImage& img, const std::vector<QRgb>& compartmentColours,
    const std::vector<std::pair<std::string, ColourPair>>&
        membraneColourPairs) {
  // construct bool grid of all boundary points
  BoundaryPixels bbg(img, compartmentColours, membraneColourPairs);
  boundaryPixelsImage = bbg.getBoundaryPixelsImage();
  for (const auto& fp : bbg.getFixedPoints()) {
    auto& f = fixedPoints.emplace_back();
    f.point = fp;
  }
  // we now have an unordered set of all boundary points
  // with points where three compartments meet identified as "fixed"
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
  for (const auto& fp : bbg.getFixedPoints()) {
    SPDLOG_TRACE("fixedPoint ({},{})", fp.x(), fp.y());
    for (const auto& startPoint : getBPNeighboursOfFP(fp, bbg)) {
      std::vector<QPoint> points{fp};
      SPDLOG_TRACE("  - ({},{})", startPoint.x(), startPoint.y());
      points.push_back(startPoint);
      // check if is membrane
      bool isMembrane = bbg.isMembrane(startPoint);
      std::size_t membraneIndex = bbg.getMembraneIndex(startPoint);
      const std::string& membraneName = bbg.getMembraneName(membraneIndex);
      SPDLOG_TRACE("  membrane: {}", membraneName);
      // visit boundary neighbours until we can't find another
      auto currPoint = bbg.getNeighbourOnBoundary(startPoint);
      bbg.visitPoint(startPoint);
      while (currPoint.has_value()) {
        QPoint p = currPoint.value();
        SPDLOG_TRACE("  - ({},{})", p.x(), p.y());
        points.push_back(p);
        currPoint = bbg.getNeighbourOnBoundary(p);
        bbg.visitPoint(p);
      }
      // look for neighbour of last point which is in a FP
      if (auto lastPoint = getFPNeighbourOfBP(points.back(), bbg);
          lastPoint.has_value()) {
        std::size_t startFP = bbg.getFixedPointIndex(fp);
        std::size_t endFP = bbg.getFixedPointIndex(lastPoint.value());
        // add this boundary index to the two FPs
        auto& l1 = fixedPoints[startFP].lines.emplace_back();
        l1.boundaryIndex = boundaries.size();
        l1.startsFromFP = true;
        l1.isMembrane = isMembrane;
        auto& l2 = fixedPoints[endFP].lines.emplace_back();
        l2.boundaryIndex = boundaries.size();
        l2.startsFromFP = false;
        l2.isMembrane = isMembrane;
        // add final fixed point to boundary line
        points.push_back(lastPoint.value());
        SPDLOG_TRACE("  -> fixedPoint ({},{})", points.back().x(),
                     points.back().y());
        SPDLOG_TRACE("  [{} connecting pixels]", points.size());
        // add line to boundaries
        boundaries.emplace_back(points, false, isMembrane, membraneName);
        boundaries.back().setFpIndices({startFP, endFP});
        SPDLOG_TRACE("  fpIndices: {}->{}", startFP, endFP);
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
  auto startPoint = getAnyBoundaryPoint(bbg);
  while (startPoint.has_value()) {
    SPDLOG_TRACE("loop start point ({},{})", startPoint.value().x(),
                 startPoint.value().y());
    bool isMembrane = bbg.isMembrane(startPoint.value());
    std::size_t membraneIndex = bbg.getMembraneIndex(startPoint.value());
    const std::string& membraneName = bbg.getMembraneName(membraneIndex);
    std::vector<QPoint> points{startPoint.value()};
    auto currPoint = bbg.getNeighbourOnBoundary(startPoint.value());
    bbg.visitPoint(startPoint.value());
    while (currPoint.has_value()) {
      QPoint p = currPoint.value();
      points.push_back(p);
      currPoint = bbg.getNeighbourOnBoundary(p);
      bbg.visitPoint(p);
    }
    SPDLOG_TRACE("  - {} points", points.size());
    SPDLOG_TRACE("  - membrane: {}", isMembrane);
    SPDLOG_TRACE("  - membrane index: {}", membraneIndex);
    SPDLOG_TRACE("  - membrane name: {}", membraneName);
    boundaries.emplace_back(points, true, isMembrane, membraneName);
    startPoint = getAnyBoundaryPoint(bbg);
  }
  expandFPs();
}

const std::vector<Boundary>& ImageBoundaries::getBoundaries() const {
  return boundaries;
}

const std::vector<FixedPoint>& ImageBoundaries::getFixedPoints() const {
  return fixedPoints;
}

const std::vector<QPointF>& ImageBoundaries::getNewFixedPoints() const {
  return newFixedPoints;
}

const QImage& ImageBoundaries::getBoundaryPixelsImage() const {
  return boundaryPixelsImage;
}

void ImageBoundaries::setMaxPoints(
    const std::vector<std::size_t>& boundaryMaxPoints) {
  for (std::size_t i = 0; i < boundaries.size(); ++i) {
    boundaries[i].setMaxPoints(boundaryMaxPoints[i]);
  }
  expandFPs();
}

std::vector<std::size_t> ImageBoundaries::setAutoMaxPoints() {
  std::vector<std::size_t> maxPoints;
  maxPoints.reserve(boundaries.size());
  for (auto& b : boundaries) {
    maxPoints.push_back(b.setMaxPoints());
  }
  expandFPs();
  return maxPoints;
}

void ImageBoundaries::setMaxPoints(std::size_t boundaryIndex,
                                   std::size_t maxPoints) {
  boundaries[boundaryIndex].setMaxPoints(maxPoints);
  expandFPs();
}

void ImageBoundaries::setMembraneWidths(
    const std::vector<double>& newMembraneWidths) {
  for (std::size_t i = 0; i < newMembraneWidths.size(); ++i) {
    if (auto& boundary = boundaries[i]; boundary.isMembrane()) {
      boundary.setMembraneWidth(newMembraneWidths[i]);
    }
  }
  expandFPs();
}

void ImageBoundaries::setMembraneWidth(std::size_t boundaryIndex,
                                       double newMembraneWidth) {
  if (auto& boundary = boundaries[boundaryIndex]; boundary.isMembrane()) {
    boundary.setMembraneWidth(newMembraneWidth);
    expandFPs();
  }
}

}  // namespace mesh
