#include "triangulate_common.hpp"
#include "boundary.hpp"
#include "logger.hpp"
#include "utils.hpp"

namespace sme::mesh {

std::size_t getOrInsertFPIndex(const QPoint &p,
                                      std::vector<QPoint> &fps) {
  // return index of item in points that matches p
  SPDLOG_TRACE("looking for point ({}, {})", p.x(), p.y());
  for (std::size_t i = 0; i < fps.size(); ++i) {
    if (fps[i] == p) {
      SPDLOG_TRACE("  -> found [{}] : ({}, {})", i, fps[i].x(), fps[i].y());
      return i;
    }
  }
  // if not found: add p to vector and return its index
  SPDLOG_TRACE("  -> added new point");
  fps.push_back(p);
  return fps.size() - 1;
}

TriangulateBoundaries::TriangulateBoundaries() = default;

TriangulateBoundaries::TriangulateBoundaries(
    const std::vector<Boundary> &inputBoundaries,
    const std::vector<std::vector<QPointF>> &interiorPoints,
    const std::vector<std::size_t> &maxTriangleAreas) {
  std::size_t nPointsUpperBound = 0;
  for (const auto &boundary : inputBoundaries) {
    nPointsUpperBound += boundary.getPoints().size();
  }
  vertices.reserve(nPointsUpperBound);
  // first add fixed points
  std::vector<QPoint> fps;
  fps.reserve(2 * inputBoundaries.size());
  for (const auto &boundary : inputBoundaries) {
    if (!boundary.isLoop()) {
      getOrInsertFPIndex(boundary.getPoints().front(), fps);
      getOrInsertFPIndex(boundary.getPoints().back(), fps);
    }
  }
  for (const auto &fp : fps) {
    SPDLOG_TRACE("- fp ({},{})", fp.x(), fp.y());
    vertices.emplace_back(fp);
  }

  // for each segment in each boundary line, add the QPoints if not already
  // present, and add the pair of point indices to the list of segment indices
  std::size_t currentIndex = vertices.size() - 1;
  for (const auto &boundary : inputBoundaries) {
    SPDLOG_TRACE("{}-point boundary", boundary.getPoints().size());
    SPDLOG_TRACE("  - loop: {}", boundary.isLoop());
    const auto &points = boundary.getPoints();
    auto &segments = boundaries.emplace_back();
    // do first segment
    if (boundary.isLoop()) {
      vertices.emplace_back(points[0]);
      ++currentIndex;
      vertices.emplace_back(points[1]);
      ++currentIndex;
      segments.push_back({currentIndex - 1, currentIndex});
    } else if (points.size() == 2) {
      segments.push_back({getOrInsertFPIndex(points.front(), fps),
                          getOrInsertFPIndex(points.back(), fps)});
    } else {
      vertices.emplace_back(points[1]);
      ++currentIndex;
      segments.push_back(
          {getOrInsertFPIndex(points.front(), fps), currentIndex});
    }
    // do intermediate segments
    for (std::size_t j = 2; j < points.size() - 1; ++j) {
      vertices.emplace_back(points[j]);
      ++currentIndex;
      segments.push_back({currentIndex - 1, currentIndex});
    }
    // do last segment
    if (boundary.isLoop()) {
      ++currentIndex;
      vertices.emplace_back(points.back());
      segments.push_back({currentIndex - 1, currentIndex});
      // for loops: also connect last point to first point
      segments.push_back({currentIndex, segments.front().start});
    } else if (points.size() > 2) {
      segments.push_back(
          {currentIndex, getOrInsertFPIndex(points.back(), fps)});
    }
#if SPDLOG_ACTIVE_LEVEL <= SPDLOG_LEVEL_TRACE
    for (const auto &seg : segments) {
      SPDLOG_TRACE("- seg {}->{} | ({},{})->({},{})", seg.start, seg.end,
                   vertices[seg.start].x(), vertices[seg.start].y(),
                   vertices[seg.end].x(), vertices[seg.end].y());
    }
#endif
  }
  // add interior point & max triangle area for each compartment
  for (std::size_t i = 0; i < interiorPoints.size(); ++i) {
    compartments.push_back(
        {interiorPoints[i], static_cast<double>(maxTriangleAreas[i])});
  }
}

} // namespace sme
