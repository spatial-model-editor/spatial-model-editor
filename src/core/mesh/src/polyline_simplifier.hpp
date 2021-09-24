#pragma once

#include "boundary.hpp"
#include <CGAL/Constrained_Delaunay_triangulation_2.h>
#include <CGAL/Constrained_triangulation_plus_2.h>
#include <CGAL/Exact_predicates_inexact_constructions_kernel.h>
#include <CGAL/Polygon_2.h>
#include <CGAL/Polyline_simplification_2/simplify.h>
#include <QImage>
#include <QPoint>
#include <QPointF>
#include <memory>
#include <string>
#include <vector>

namespace sme::mesh {

using CGALKernel = CGAL::Exact_predicates_inexact_constructions_kernel;
using CGALVb = CGAL::Polyline_simplification_2::Vertex_base_2<CGALKernel>;
using CGALFb = CGAL::Constrained_triangulation_face_base_2<CGALKernel>;
using CGALTds = CGAL::Triangulation_data_structure_2<CGALVb, CGALFb>;
using CGALCdt =
    CGAL::Constrained_Delaunay_triangulation_2<CGALKernel, CGALTds,
                                               CGAL::Exact_predicates_tag>;
using CGALCt = CGAL::Constrained_triangulation_plus_2<CGALCdt>;

/**
 * @brief Simplify a set of boundary lines
 *
 * Simplify a set of lines by removing vertices without causing any new
 * intersections: topology-preserving polyline simplification.
 * https://doc.cgal.org/latest/Polyline_simplification_2
 */
class PolylineSimplifier {
private:
  std::vector<CGALCt::Constraint_id> constraintIds;
  CGALCt ct;
  void initCt(const std::vector<Boundary> &boundaries);

public:
  explicit PolylineSimplifier(const std::vector<Boundary> &boundaries);
  explicit PolylineSimplifier() = default;
  PolylineSimplifier(const PolylineSimplifier &) = default;
  PolylineSimplifier &operator=(const PolylineSimplifier &) = default;
  // delete move constructors to avoid CGAL::Constrained_triangulation_plus_2
  // move-constructor, which leaks memory
  PolylineSimplifier(PolylineSimplifier &&) = delete;
  PolylineSimplifier &operator=(PolylineSimplifier &&) = delete;
  ~PolylineSimplifier() = default;
  void setBoundaries(const std::vector<Boundary> &boundaries);
  [[nodiscard]] std::size_t getMaxPoints() const;
  void setMaxPoints(std::vector<Boundary> &boundaries, std::size_t maxPoints);
};

} // namespace sme::mesh
