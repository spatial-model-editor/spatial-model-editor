#include "boundary.hpp"
#include "line_simplifier.hpp"
#include <algorithm>
#include <cmath>
#include <iterator>
#include <utility>

namespace mesh {

bool Boundary::isLoop() const { return loop; }

bool Boundary::isMembrane() const { return membrane; }

bool Boundary::isValid() const { return valid; }

const std::vector<QPoint> &Boundary::getPoints() const { return points; }

const FpIndices &Boundary::getFpIndices() const { return fpIndices; }

void Boundary::setFpIndices(const FpIndices &indices) { fpIndices = indices; }

std::size_t Boundary::getMaxPoints() const { return maxPoints; }

void Boundary::setMaxPoints(std::size_t nMaxPoints) {
  maxPoints = nMaxPoints;
  lineSimplifier.getSimplifiedLine(points, maxPoints);
}

std::size_t Boundary::setMaxPoints() {
  lineSimplifier.getSimplifiedLine(points);
  maxPoints = points.size();
  return maxPoints;
}

const std::string &Boundary::getMembraneId() const { return membraneID; }

std::size_t Boundary::getMembraneIndex() const { return membraneIndex; }

Boundary::Boundary(const std::vector<QPoint> &boundaryPoints, bool isClosedLoop,
                   bool isMembraneCompartment, const std::string &membraneName,
                   std::size_t membraneIndex)
    : loop{isClosedLoop}, membrane{isMembraneCompartment},
      membraneID(membraneName), membraneIndex{membraneIndex},
      lineSimplifier(boundaryPoints, isClosedLoop) {
  valid = lineSimplifier.isValid();
  if (!valid) {
    return;
  }
  setMaxPoints();
}

} // namespace mesh
