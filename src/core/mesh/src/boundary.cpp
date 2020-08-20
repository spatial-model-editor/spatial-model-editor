#include "boundary.hpp"
#include "line_simplifier.hpp"
#include <algorithm>
#include <cmath>
#include <iterator>
#include <utility>

namespace mesh {

static QPointF getNormalUnitVector(const QPoint &p0, const QPoint &pNext) {
  QPointF normal;
  auto dx = static_cast<double>(pNext.x() - p0.x());
  auto dy = static_cast<double>(pNext.y() - p0.y());
  double norm = std::hypot(dx, dy);
  normal = QPointF(-dy / norm, dx / norm);
  return normal;
}

static QPointF getAvgNormalUnitVector(const QPoint &pPrev, const QPoint &p0,
                                      const QPoint &pNext) {
  QPointF normal = getNormalUnitVector(p0, pNext);
  normal += getNormalUnitVector(pPrev, p0);
  normal /= std::hypot(normal.x(), normal.y());
  return normal;
}

void Boundary::constructMembraneBoundarySegment(std::size_t index,
                                                const QPointF &normal) {
  auto n = 0.5 * membraneWidth * normal;
  outerPoints.push_back(points[index] + n);
  innerPoints.push_back(points[index] - n);
}

void Boundary::constructMembraneBoundaries() {
  innerPoints.clear();
  outerPoints.clear();
  if (!membrane) {
    std::copy(points.cbegin(), points.cend(), std::back_inserter(innerPoints));
    return;
  }
  std::size_t nP = points.size();
  QPointF normal;
  // do first point
  if (loop) {
    normal = getAvgNormalUnitVector(points[nP - 1], points[0], points[1]);
  } else {
    normal = getNormalUnitVector(points[0], points[1]);
  }
  constructMembraneBoundarySegment(0, normal);
  // all other intermediate points
  for (std::size_t i = 1; i < nP - 1; ++i) {
    normal = getAvgNormalUnitVector(points[i - 1], points[i], points[i + 1]);
    constructMembraneBoundarySegment(i, normal);
  }
  // do last point
  if (loop) {
    normal = getAvgNormalUnitVector(points[nP - 2], points[nP - 1], points[0]);
  } else {
    normal = getNormalUnitVector(points[nP - 2], points[nP - 1]);
  }
  constructMembraneBoundarySegment(nP - 1, normal);
}

bool Boundary::isLoop() const { return loop; }

bool Boundary::isMembrane() const { return membrane; }

bool Boundary::isValid() const { return valid; }

const std::vector<QPoint> &Boundary::getPoints() const { return points; }

const std::vector<QPointF> &Boundary::getInnerPoints() const {
  return innerPoints;
}

void Boundary::setInnerStartPoint(const QPointF &point, std::size_t index) {
  innerPoints.front() = point;
  innerFpIndices.startPoint = index;
}

void Boundary::setInnerEndPoint(const QPointF &point, std::size_t index) {
  innerPoints.back() = point;
  innerFpIndices.endPoint = index;
}

const std::vector<QPointF> &Boundary::getOuterPoints() const {
  return outerPoints;
}

void Boundary::setOuterStartPoint(const QPointF &point, std::size_t index) {
  outerPoints.front() = point;
  outerFpIndices.startPoint = index;
}

void Boundary::setOuterEndPoint(const QPointF &point, std::size_t index) {
  outerPoints.back() = point;
  outerFpIndices.endPoint = index;
}

const FpIndices &Boundary::getFpIndices() const { return fpIndices; }

void Boundary::setFpIndices(const FpIndices &indices) { fpIndices = indices; }

const FpIndices &Boundary::getInnerFpIndices() const { return innerFpIndices; }

const FpIndices &Boundary::getOuterFpIndices() const { return outerFpIndices; }

std::size_t Boundary::getMaxPoints() const { return maxPoints; }

void Boundary::setMaxPoints(std::size_t nMaxPoints) {
  maxPoints = nMaxPoints;
  lineSimplifier.getSimplifiedLine(points, maxPoints);
  constructMembraneBoundaries();
}

std::size_t Boundary::setMaxPoints() {
  lineSimplifier.getSimplifiedLine(points);
  maxPoints = points.size();
  constructMembraneBoundaries();
  return maxPoints;
}

double Boundary::getMembraneWidth() const { return membraneWidth; }

void Boundary::setMembraneWidth(double newMembraneWidth) {
  membraneWidth = newMembraneWidth;
  setMaxPoints(maxPoints);
}

const std::string &Boundary::getMembraneId() const { return membraneID; }

Boundary::Boundary(const std::vector<QPoint> &boundaryPoints, bool isClosedLoop,
                   bool isMembraneCompartment, std::string membraneName)
    : loop{isClosedLoop}, membrane{isMembraneCompartment},
      membraneID(std::move(membraneName)),
      lineSimplifier(boundaryPoints, isClosedLoop) {
  valid = lineSimplifier.isValid();
  if (!valid) {
    return;
  }
  innerPoints.reserve(lineSimplifier.maxPoints());
  outerPoints.reserve(lineSimplifier.maxPoints());
  setMaxPoints();
}

} // namespace mesh
