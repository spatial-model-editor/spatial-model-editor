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

static double getAngleFromLine(const QPointF &p0, const QPointF &p) {
  // get anticlockwise angle of p0->p line relative to horizontal x axis
  return modAngle(std::atan2(p.y() - p0.y(), p.x() - p0.x()));
}

static QPointF makePoint(double radius, double angle, const QPointF &x0) {
  return x0 + QPointF(radius * std::cos(angle), radius * std::sin(angle));
}

static bool fpHasMembrane(const FixedPoint &fp) {
  return std::any_of(fp.lines.cbegin(), fp.lines.cend(),
                     [](const auto &line) { return line.isMembrane; });
}

double ImageBoundaries::getFpRadius(const FixedPoint &fp) const {
  double radius = std::numeric_limits<double>::max();
  for (const auto &line : fp.lines) {
    if (const auto &b = boundaries[line.boundaryIndex]; b.isMembrane()) {
      radius = std::min(radius, b.getMembraneWidth());
    }
  }
  return radius / std::sqrt(2.0);
}

void ImageBoundaries::updateFpAngles(FixedPoint &fp) {
  for (auto &line : fp.lines) {
    const auto &points = boundaries[line.boundaryIndex].getPoints();
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
            [](const auto &a, const auto &b) { return a.angle < b.angle; });
}

static double midpointAngle(double a1, double a2) {
  return modAngle(modAngle(a1) + 0.5 * modAngle(a2 - a1));
}

static std::size_t getOrInsertIndex(const QPointF &p,
                                    std::vector<QPointF> &points) {
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

void ImageBoundaries::expandFP(FixedPoint &fp) {
  SPDLOG_TRACE("fp: ({},{})", fp.point.x(), fp.point.y());
  double radius = getFpRadius(fp);
  // get angle of each boundary line to the FP
  updateFpAngles(fp);
  for (std::size_t iLine = 0; iLine < fp.lines.size(); ++iLine) {
    const auto &line = fp.lines[iLine];
    const auto &linePlus = fp.lines[(iLine + 1) % fp.lines.size()];
    const auto &lineMinus =
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
    auto &b = boundaries[line.boundaryIndex];
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
  for (auto &fp : fixedPoints) {
    if (fpHasMembrane(fp)) {
      expandFP(fp);
    }
  }
}

void ImageBoundaries::constructLines(
    const QImage &img, const std::vector<QRgb> &compartmentColours,
    const std::vector<std::pair<std::string, ColourPair>>
        &membraneColourPairs) {
  auto contours = Contours(img, compartmentColours, membraneColourPairs);
  fixedPoints = std::move(contours.getFixedPoints());
  boundaries = std::move(contours.getBoundaries());
  boundaryPixelsImage = contours.getBoundaryPixelsImage();
}

ImageBoundaries::ImageBoundaries() = default;

ImageBoundaries::ImageBoundaries(
    const QImage &img, const std::vector<QRgb> &compartmentColours,
    const std::vector<std::pair<std::string, ColourPair>>
        &membraneColourPairs) {
  constructLines(img, compartmentColours, membraneColourPairs);
  expandFPs();
}

ImageBoundaries::~ImageBoundaries() = default;

const std::vector<Boundary> &ImageBoundaries::getBoundaries() const {
  return boundaries;
}

const std::vector<FixedPoint> &ImageBoundaries::getFixedPoints() const {
  return fixedPoints;
}

const std::vector<QPointF> &ImageBoundaries::getNewFixedPoints() const {
  return newFixedPoints;
}

const QImage &ImageBoundaries::getBoundaryPixelsImage() const {
  return boundaryPixelsImage;
}

void ImageBoundaries::setMaxPoints(
    const std::vector<std::size_t> &boundaryMaxPoints) {
  for (std::size_t i = 0; i < boundaries.size(); ++i) {
    boundaries[i].setMaxPoints(boundaryMaxPoints[i]);
  }
  expandFPs();
}

std::vector<std::size_t> ImageBoundaries::setAutoMaxPoints() {
  std::vector<std::size_t> maxPoints;
  maxPoints.reserve(boundaries.size());
  for (auto &b : boundaries) {
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
    const std::vector<double> &newMembraneWidths) {
  for (std::size_t i = 0; i < newMembraneWidths.size(); ++i) {
    if (auto &boundary = boundaries[i]; boundary.isMembrane()) {
      boundary.setMembraneWidth(newMembraneWidths[i]);
    }
  }
  expandFPs();
}

void ImageBoundaries::setMembraneWidth(std::size_t boundaryIndex,
                                       double newMembraneWidth) {
  if (auto &boundary = boundaries[boundaryIndex]; boundary.isMembrane()) {
    boundary.setMembraneWidth(newMembraneWidth);
    expandFPs();
  }
}

} // namespace mesh
