#include "boundary.hpp"
#include "line_simplifier.hpp"
#include <algorithm>
#include <utility>

namespace sme::mesh {

bool Boundary::isLoop() const { return lineSimplifier.isLoop(); }

bool Boundary::isValid() const { return lineSimplifier.isValid(); }

const std::vector<QPoint> &Boundary::getPoints() const { return points; }

const std::vector<QPoint> &Boundary::getAllPoints() const {
  return lineSimplifier.getAllVertices();
}

std::size_t Boundary::getMaxPoints() const { return maxPoints; }

void Boundary::setPoints(std::vector<QPoint> &&simplifiedPoints) {
  points = std::move(simplifiedPoints);
}

void Boundary::setMaxPoints(std::size_t nMaxPoints) {
  maxPoints = nMaxPoints;
  lineSimplifier.getSimplifiedLine(points, maxPoints);
}

std::size_t Boundary::setMaxPoints() {
  lineSimplifier.getSimplifiedLine(points);
  maxPoints = points.size();
  return maxPoints;
}

Boundary::Boundary(const std::vector<QPoint> &boundaryPoints, bool isClosedLoop)
    : lineSimplifier(boundaryPoints, isClosedLoop) {
  if (!lineSimplifier.isValid()) {
    return;
  }
  setMaxPoints();
}

} // namespace sme::mesh
