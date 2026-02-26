#pragma once

#include "boundary.hpp"
#include <QPointF>
#include <algorithm>
#include <array>
#include <cstddef>
#include <string>
#include <vector>

namespace sme::mesh {

/**
 * @brief Edge segment in triangulation boundary input.
 */
struct TriangulateBoundarySegment {
  std::size_t start;
  std::size_t end;
};

/**
 * @brief Collection of boundary segments.
 */
using TriangulateBoundarySegments = std::vector<TriangulateBoundarySegment>;
/**
 * @brief Triangle vertex index triple.
 */
using TriangulateTriangleIndex = std::array<std::size_t, 3>;

/**
 * @brief Per-compartment triangulation constraints.
 */
struct TriangulateCompartment {
  /**
   * @brief Interior seed points for region identification.
   */
  std::vector<QPointF> interiorPoints;
  /**
   * @brief Maximum allowed triangle area.
   */
  double maxTriangleArea;
};

/**
 * @brief Return existing fixed-point index or insert new point.
 */
std::size_t getOrInsertFPIndex(const QPoint &p, std::vector<QPoint> &fps);

/**
 * @brief Triangulation-ready representation of boundaries and compartments.
 */
struct TriangulateBoundaries {
  /**
   * @brief Construct empty boundary set.
   */
  TriangulateBoundaries();
  /**
   * @brief Build triangulation boundaries from simplified model boundaries.
   */
  TriangulateBoundaries(const std::vector<Boundary> &inputBoundaries,
                        const std::vector<std::vector<QPointF>> &interiorPoints,
                        const std::vector<std::size_t> &maxTriangleAreas);
  /**
   * @brief Unique vertex coordinates.
   */
  std::vector<QPointF> vertices;
  /**
   * @brief Boundary segments indexed into ``vertices``.
   */
  std::vector<TriangulateBoundarySegments> boundaries;
  /**
   * @brief Per-compartment triangulation metadata.
   */
  std::vector<TriangulateCompartment> compartments;
};

} // namespace sme::mesh
