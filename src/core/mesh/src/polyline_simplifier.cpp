#include "polyline_simplifier.hpp"
#include "boundary.hpp"
#include "logger.hpp"
#include <algorithm>

namespace sme::mesh {

PolylineSimplifier::PolylineSimplifier(
    const std::vector<Boundary> &boundaries) {
  setBoundaries(boundaries);
}

void PolylineSimplifier::setBoundaries(
    const std::vector<Boundary> &boundaries) {
  ct.clear();
  constraintIds.clear();
  for (auto &boundary : boundaries) {
    std::vector<CGALCt::Point> v;
    auto n{boundary.getAllPoints().size()};
    v.reserve(n);
    for (const auto &p : boundary.getAllPoints()) {
      v.emplace_back(p.x(), p.y());
    }
    constraintIds.push_back(
        ct.insert_constraint(v.begin(), v.end(), boundary.isLoop()));
  }
}

std::size_t PolylineSimplifier::getMaxPoints() const {
  return ct.number_of_vertices();
}

static void
updatePoints(const CGALCt &ct,
             const std::vector<CGALCt::Constraint_id> &constraintIds,
             std::vector<Boundary> &boundaries) {
  for (std::size_t i = 0; i < boundaries.size(); ++i) {
    std::vector<QPoint> points;
    auto vertices{ct.vertices_in_constraint(constraintIds[i])};
    points.reserve(vertices.size());
    for (auto vertex : vertices) {
      points.emplace_back(static_cast<int>(vertex->point().x()),
                          static_cast<int>(vertex->point().y()));
    }
    if (boundaries[i].isLoop()) {
      points.pop_back();
    }
    boundaries[i].setPoints(std::move(points));
  }
}

void PolylineSimplifier::setMaxPoints(std::vector<Boundary> &boundaries,
                                      std::size_t maxPoints) {
  namespace PS = CGAL::Polyline_simplification_2;
  if (boundaries.empty()) {
    return;
  }
  bool canReuseCt{boundaries.size() == constraintIds.size() && maxPoints != 0 &&
                  maxPoints <= ct.number_of_vertices()};
  if (!canReuseCt) {
    setBoundaries(boundaries);
  }
  if (maxPoints == 0) {
    constexpr double maxAllowedCost{5.0};
    PS::simplify(ct, PS::Squared_distance_cost(),
                 PS::Stop_above_cost_threshold(maxAllowedCost), false);
  } else {
    PS::simplify(ct, PS::Squared_distance_cost(),
                 PS::Stop_below_count_threshold(maxPoints), false);
  }
  updatePoints(ct, constraintIds, boundaries);
}

} // namespace sme::mesh
