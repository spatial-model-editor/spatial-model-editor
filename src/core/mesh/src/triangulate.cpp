#include "triangulate.hpp"
#include "logger.hpp"
#include "triangle.hpp"
#include "utils.hpp"
#include <QImage>
#include <QPainter>
#include <QRectF>
#include <QString>
#include <cmath>
#include <initializer_list>
#include <iterator>
#include <limits>
#include <memory>
#include <numeric>
#include <stdlib.h>
#include <string>
#include <utility>

namespace mesh {

static void setPointList(triangle::triangulateio &in,
                         const std::vector<QPointF> &boundaryPoints) {
  free(in.pointlist);
  in.pointlist = nullptr;
  in.pointlist =
      static_cast<double *>(malloc(2 * boundaryPoints.size() * sizeof(double)));
  in.numberofpoints = static_cast<int>(boundaryPoints.size());
  double *d = in.pointlist;
  for (const auto &p : boundaryPoints) {
    *d = static_cast<double>(p.x());
    ++d;
    *d = static_cast<double>(p.y());
    ++d;
  }
}

static void setSegmentList(triangle::triangulateio &in,
                           const std::vector<BoundarySegments> &boundaries) {
  free(in.segmentlist);
  in.segmentlist = nullptr;
  free(in.segmentmarkerlist);
  in.segmentmarkerlist = nullptr;
  std::size_t nSegments{0};
  for (const auto &boundarySegments : boundaries) {
    nSegments += boundarySegments.size();
  }
  if (nSegments == 0) {
    return;
  }
  in.segmentlist = static_cast<int *>(malloc(2 * nSegments * sizeof(int)));
  in.numberofsegments = static_cast<int>(nSegments);
  in.segmentmarkerlist = static_cast<int *>(malloc(nSegments * sizeof(int)));
  int *seg = in.segmentlist;
  int *segm = in.segmentmarkerlist;
  // 2-based indexing of boundaries
  // 0 and 1 may be used by triangle library
  int boundaryMarker = 2;
  for (const auto &boundary : boundaries) {
    for (const auto &segment : boundary) {
      *seg = static_cast<int>(segment.start);
      ++seg;
      *seg = static_cast<int>(segment.end);
      ++seg;
      *segm = boundaryMarker;
      ++segm;
    }
    ++boundaryMarker;
  }
}

static void setRegionList(triangle::triangulateio &in,
                          const std::vector<Compartment> &compartments) {
  // 1-based indexing of compartments
  // 0 is then used for triangles that are not part of a compartment
  free(in.regionlist);
  in.regionlist = nullptr;
  if (!compartments.empty()) {
    in.regionlist =
        static_cast<double *>(malloc(4 * compartments.size() * sizeof(double)));
    in.numberofregions = static_cast<int>(compartments.size());
    double *r = in.regionlist;
    int i = 1;
    for (const auto &compartment : compartments) {
      *r = static_cast<double>(compartment.interiorPoint.x());
      ++r;
      *r = static_cast<double>(compartment.interiorPoint.y());
      ++r;
      *r = static_cast<double>(i); // compartment index + 1
      ++r;
      *r = compartment.maxTriangleArea;
      ++r;
      ++i;
    }
  }
}

static void setHoleList(triangle::triangulateio &in,
                        const std::vector<QPointF> &holes) {
  free(in.holelist);
  in.holelist = nullptr;
  if (!holes.empty()) {
    in.holelist =
        static_cast<double *>(malloc(2 * holes.size() * sizeof(double)));
    in.numberofholes = static_cast<int>(holes.size());
    double *h = in.holelist;
    for (const auto &hole : holes) {
      *h = static_cast<double>(hole.x());
      ++h;
      *h = static_cast<double>(hole.y());
      ++h;
    }
  }
}

static triangle::triangulateio
toTriangulateio(const TriangulateBoundaries &tid) {
  triangle::triangulateio io;
  setPointList(io, tid.boundaryPoints);
  setSegmentList(io, tid.boundaries);
  setRegionList(io, tid.compartments);
  return io;
}

// swap segment at `index` with the first subsequent segment that contains
// `value`, flipping segment if needed such that start of segment is `value`
static void swapSegment(BoundarySegments &segments, std::size_t index,
                        std::size_t value) {
  for (std::size_t i = index; i < segments.size(); ++i) {
    if (segments[i].start == value) {
      // found segment
      std::swap(segments[index], segments[i]);
      SPDLOG_TRACE("found ({}, {})", segments[index].start,
                   segments[index].end);
      return;
    }
    if (segments[i].end == value) {
      // found segment but needs to be reversed
      std::swap(segments[index], segments[i]);
      SPDLOG_TRACE("found ({}, {})", segments[index].start,
                   segments[index].start);
      std::swap(segments[index].start, segments[index].end);
      SPDLOG_TRACE("swapped to ({}, {})", segments[index].start,
                   segments[index].end);
      return;
    }
  }
}

static void orderSegmentsInPlace(BoundarySegments &segments,
                                 std::size_t firstValue) {
  swapSegment(segments, 0, firstValue);
  if (segments.front().start != firstValue) {
    SPDLOG_WARN("failed to find starting point {} in boundary segments",
                firstValue);
  }
  for (std::size_t i = 1; i < segments.size(); ++i) {
    swapSegment(segments, i, segments[i - 1].end);
  }
#if SPDLOG_ACTIVE_LEVEL <= SPDLOG_LEVEL_TRACE
  for (const auto &s : segments) {
    SPDLOG_TRACE("- ({}, {})", s.start, s.end);
  }
#endif
  return;
}

static void unitNormaliseInPlace(QPointF &p) {
  double n = std::hypot(p.x(), p.y());
  p /= n;
}

static QPointF getNormalUnitVector(const BoundarySegment &seg,
                                   const std::vector<QPointF> &points) {
  QPointF n;
  QPointF d = points[seg.end] - points[seg.start];
  n = QPointF(-d.y(), d.x());
  unitNormaliseInPlace(n);
  return n;
}

static QPointF getAvgNormalUnitVector(const BoundarySegment &seg,
                                      const BoundarySegment &segNext,
                                      const std::vector<QPointF> &points) {
  QPointF n = getNormalUnitVector(seg, points);
  n += getNormalUnitVector(segNext, points);
  unitNormaliseInPlace(n);
  return n;
}

#if SPDLOG_ACTIVE_LEVEL <= SPDLOG_LEVEL_TRACE
static QRectF getBoundingRectangle(const std::vector<QPointF> &points) {
  QRectF r;
  double minX = std::numeric_limits<double>::max();
  double minY = std::numeric_limits<double>::max();
  double maxX = std::numeric_limits<double>::min();
  double maxY = std::numeric_limits<double>::min();
  for (const auto &p : points) {
    double x = p.x();
    double y = p.y();
    minX = std::min(minX, x);
    maxX = std::max(maxX, x);
    minY = std::min(minY, y);
    maxY = std::max(maxY, y);
  }
  r.setLeft(minX);
  r.setRight(maxX);
  r.setTop(minY);
  r.setBottom(maxY);
  return r;
}

static void debugDrawPointsAndBoundaries(const TriangulateBoundaries &tid,
                                         const QString &filename) {
  QImage img(800, 800, QImage::Format_ARGB32_Premultiplied);
  img.fill(0);
  QPainter painter(&img);
  const auto &bp = tid.boundaryPoints;
  auto rect = getBoundingRectangle(bp);
  auto p0 = rect.topLeft();
  double scale =
      std::min(img.width() / rect.width(), img.height() / rect.height());
  SPDLOG_TRACE("Points:");
  for (std::size_t i = 0; i < bp.size(); ++i) {
    painter.drawEllipse(scale * bp[i] + p0, 2, 2);
    SPDLOG_TRACE("  - [{}] ({},{})", i, bp[i].x(), bp[i].y());
  }
  for (std::size_t i = 0; i < tid.boundaries.size(); ++i) {
    painter.setPen(QPen(utils::indexedColours()[i], 2));
    const auto &b = tid.boundaries[i];
    SPDLOG_TRACE("Boundary {}:", i);
    for (std::size_t j = 0; j < b.size(); ++j) {
      painter.drawLine(scale * bp[b[j].start] + p0, scale * bp[b[j].end] + p0);
      SPDLOG_TRACE("  - [{}] ([{}]->[{}]): ({},{})->({},{})", j, b[j].start,
                   b[j].end, bp[b[j].start].x(), bp[b[j].start].y(),
                   bp[b[j].end].x(), bp[b[j].end].y());
    }
  }
  painter.end();
  img.mirrored(false, true).save(filename);
}
#endif

// http://www.cs.cmu.edu/~quake/triangle.switch.html
//  - Q: no printf output
//       (use V, VV, VVV for more verbose output)
//  - z: 0-based indexing
//  - p: supply Planar Straight Line Graph with vertices, segments, etc
//  - j: remove unused vertices from output
//       NOTE not doing this as it may change indices of existing points
//  - q: generate quality triangles
//       (e.g. q20 adds vertices such that triangle angles are between 20 and
//       180-2*20 degrees)
//  - A: regional attribute (identify compartment for each triangle)
//  - a: max triangle area for each compartment
const char *const triangleFlags = "Qzpq20.5Aa";

static TriangulateBoundaries
addSteinerPoints(const TriangulateBoundaries &tid) {
  TriangulateBoundaries newTid{tid};
  for (auto &boundary : newTid.boundaries) {
    boundary.clear();
  }
#if SPDLOG_ACTIVE_LEVEL <= SPDLOG_LEVEL_TRACE
  debugDrawPointsAndBoundaries(tid, "lines_simplify.png");
#endif
  auto in = toTriangulateio(tid);
  triangle::triangulateio out;
  triangle::triangulate(triangleFlags, &in, &out, nullptr);
  SPDLOG_DEBUG("Initial triangulation: {} segments, {} points",
               out.numberofsegments, out.numberofpoints);
  // triangulation adds non-boundary points
  // re-index points excluding these non-boundary points
  constexpr std::size_t nullIndex = std::numeric_limits<std::size_t>::max();
  std::vector<std::size_t> pointIndex(
      static_cast<std::size_t>(out.numberofpoints), nullIndex);
  // original points maintain their original indices
  std::size_t currentIndex = tid.boundaryPoints.size();
  std::iota(
      pointIndex.begin(),
      pointIndex.begin() +
          static_cast<std::vector<std::size_t>::difference_type>(currentIndex),
      0);
  // get boundary segments
  for (int iSeg = 0; iSeg < out.numberofsegments; ++iSeg) {
    auto start = static_cast<std::size_t>(out.segmentlist[2 * iSeg]);
    auto end = static_cast<std::size_t>(out.segmentlist[2 * iSeg + 1]);
    auto boundary = static_cast<std::size_t>(out.segmentmarkerlist[iSeg] - 2);
    for (auto iPoint : {start, end}) {
      if (pointIndex[iPoint] == nullIndex) {
        // add boundary point if not already present
        pointIndex[iPoint] = currentIndex;
        ++currentIndex;
        newTid.boundaryPoints.emplace_back(out.pointlist[2 * iPoint],
                                           out.pointlist[2 * iPoint + 1]);
      }
    }
    // add segment
    newTid.boundaries[boundary].push_back({pointIndex[start], pointIndex[end]});
  }
  // re-order segments so each boundary starts with same point as originally
  for (std::size_t i = 0; i < tid.boundaries.size(); ++i) {
    orderSegmentsInPlace(newTid.boundaries[i], tid.boundaries[i].front().start);
  }
  SPDLOG_DEBUG("After re-indexing:");
  SPDLOG_DEBUG("  - {} points", newTid.boundaryPoints.size());
  for (const auto &s : newTid.boundaries) {
    SPDLOG_DEBUG("  - {} segment boundary", s.size());
  }
#if SPDLOG_ACTIVE_LEVEL <= SPDLOG_LEVEL_TRACE
  debugDrawPointsAndBoundaries(newTid, "lines_add_steiner.png");
#endif
  return newTid;
}

static QPointF findCentroid(const RectangleIndex &rectIndex,
                            const std::vector<QPointF> &points) {
  return std::accumulate(
      rectIndex.cbegin(), rectIndex.cend(), QPointF(0, 0),
      [&p = points](QPointF a, std::size_t i) { return a + 0.25 * p[i]; });
}

static std::vector<std::size_t> replaceFPs(TriangulateBoundaries &tid,
                                           const TriangulateFixedPoints &tfp) {
  // add new FPs to boundary points:
  std::vector<std::size_t> newFPindex(tfp.newFPs.size(), 0);
  //  - first overwrite existing old FPs
  std::copy_n(tfp.newFPs.cbegin(), tfp.nFPs, tid.boundaryPoints.begin());
  using diff = std::vector<std::size_t>::difference_type;
  std::iota(newFPindex.begin(),
            newFPindex.begin() + static_cast<diff>(tfp.nFPs), 0);
  //  - append the rest to the end
  std::iota(newFPindex.begin() + static_cast<diff>(tfp.nFPs),
            newFPindex.begin() + static_cast<diff>(tfp.newFPs.size()),
            tid.boundaryPoints.size());
  std::copy(tfp.newFPs.cbegin() +
                static_cast<std::vector<QPointF>::difference_type>(tfp.nFPs),
            tfp.newFPs.end(), std::back_inserter(tid.boundaryPoints));
  // return map from newFP index to its index in tri
  return newFPindex;
}

static void replaceFPIndices(TriangulateBoundaries &newTid,
                             std::vector<QPointF> &originalPoints,
                             const TriangulateBoundaries &tid,
                             const std::vector<std::size_t> &newFPindex) {
  // replace oldFP indices in boundary segments with new innerFPIndices
  for (const auto &b : tid.boundaryProperties) {
    if (!b.isLoop) {
      auto i = b.boundaryIndex;
      auto oldStart = tid.boundaries.at(i).front().start;
      auto oldEnd = tid.boundaries.at(i).back().end;
      auto &newStart = newTid.boundaries.at(i).front().start;
      auto &newEnd = newTid.boundaries.at(i).back().end;
      SPDLOG_TRACE("boundary[{}]: ", i);
      SPDLOG_TRACE("  - start FP was [{}] ({}, {})", oldStart,
                   tid.boundaryPoints.at(oldStart).x(),
                   tid.boundaryPoints.at(oldStart).y());
      newStart = newFPindex.at(b.innerFPIndices.front());
      originalPoints.at(newStart) = tid.boundaryPoints.at(oldStart);
      SPDLOG_TRACE("    -> now [{}] ({}, {})", newStart,
                   newTid.boundaryPoints.at(newStart).x(),
                   newTid.boundaryPoints.at(newStart).y());
      SPDLOG_TRACE("  - end FP was [{}] ({}, {})", oldEnd,
                   tid.boundaryPoints.at(oldEnd).x(),
                   tid.boundaryPoints.at(oldEnd).y());
      newEnd = newFPindex.at(b.innerFPIndices.back());
      originalPoints.at(newEnd) = tid.boundaryPoints.at(oldEnd);
      SPDLOG_TRACE("    -> now [{}] ({}, {})", newEnd,
                   newTid.boundaryPoints.at(newEnd).x(),
                   newTid.boundaryPoints.at(newEnd).y());
    }
  }
#if SPDLOG_ACTIVE_LEVEL <= SPDLOG_LEVEL_TRACE
  debugDrawPointsAndBoundaries(newTid, "lines_split_endpoints.png");
#endif
}

TriangulateBoundaries
Triangulate::addMembranes(const TriangulateBoundaries &tb,
                          const TriangulateFixedPoints &tfp) {
  TriangulateBoundaries newTid = addSteinerPoints(tb);
  // maintain copy of these points for calculating normal unit vectors
  auto originalPoints = newTid.boundaryPoints;
  // ensure it has space for the new FPs
  originalPoints.insert(originalPoints.end(), tfp.newFPs.size() - tfp.nFPs, {});
  auto newFPindex = replaceFPs(newTid, tfp);
  replaceFPIndices(newTid, originalPoints, tb, newFPindex);
  // for each membrane segment
  //  - move existing non-FP segments in perpendicular direction to form inner
  //  boundary
  //  - add outer boundary points shifted in opposite direction to inner points
  //  - add outer membrane segment
  //  - add rectangle segment connecting inner and outer membrane segments
  for (const auto &b : tb.boundaryProperties) {
    if (const auto &membrane = b; b.isMembrane) {
      auto &outer = newTid.boundaries.emplace_back();
      auto &inner = newTid.boundaries[membrane.boundaryIndex];
      std::size_t prevOuterPointIndex;
      SPDLOG_DEBUG(
          "  - adding outer membrane around {}-point boundary[{}], width {}",
          inner.size(), membrane.boundaryIndex, membrane.width);
      auto &rect = rectangleIndices.emplace_back();
      // move first inner point and add first outer point
      if (membrane.isLoop) {
        auto n =
            getAvgNormalUnitVector(inner.back(), inner.front(), originalPoints);
        n *= 0.5 * membrane.width;
        // move inner point
        newTid.boundaryPoints[inner.front().start] -= n;
        // add outer point
        prevOuterPointIndex = newTid.boundaryPoints.size();
        newTid.boundaryPoints.push_back(originalPoints[inner.front().start] +
                                        n);
      } else {
        // outer point already exists
        prevOuterPointIndex = newFPindex[membrane.outerFPIndices[0]];
        // add segment to cap this end of the membrane
        outer.push_back({inner.front().start, prevOuterPointIndex});
      }
      // do all segments except last one
      for (std::size_t iSeg = 0; iSeg < inner.size() - 1; ++iSeg) {
        const auto &seg = inner[iSeg];
        auto n = getAvgNormalUnitVector(seg, inner[iSeg + 1], originalPoints);
        n *= 0.5 * membrane.width;
        // move inner point
        newTid.boundaryPoints[seg.end] -= n;
        // add outer point
        auto outerPointIndex = newTid.boundaryPoints.size();
        newTid.boundaryPoints.push_back(originalPoints[seg.end] + n);
        // add outer segment
        outer.push_back({prevOuterPointIndex, outerPointIndex});
        // add rectangle indices
        rect.push_back(
            {seg.start, seg.end, outerPointIndex, prevOuterPointIndex});
        prevOuterPointIndex = outerPointIndex;
      }
      // do last segment: no new points required
      if (membrane.isLoop) {
        outer.push_back({outer.back().end, outer.front().start});
      } else {
        outer.push_back(
            {prevOuterPointIndex, newFPindex[membrane.outerFPIndices[1]]});
      }
      // add rectangle indices
      rect.push_back({inner.back().start, inner.back().end, outer.back().end,
                      outer.back().start});
      if (!membrane.isLoop) {
        // add segment to cap this end of the membrane
        outer.push_back({outer.back().end, inner.back().end});
      }
    }
  }
  SPDLOG_DEBUG("After constructing membranes:");
  SPDLOG_DEBUG("  - {} points", newTid.boundaryPoints.size());
  for (const auto &s : newTid.boundaries) {
    SPDLOG_DEBUG("  - {} segment boundary ({}->{})", s.size(), s.front().start,
                 s.back().end);
  }
#if SPDLOG_ACTIVE_LEVEL <= SPDLOG_LEVEL_TRACE
  debugDrawPointsAndBoundaries(newTid, "lines_add_membranes.png");
#endif
  return newTid;
}

static std::vector<QPointF>
getPointsFromTriangulateio(const triangle::triangulateio &io) {
  std::vector<QPointF> points;
  points.reserve(static_cast<std::size_t>(io.numberofpoints));
  for (int i = 0; i < io.numberofpoints; ++i) {
    double x = io.pointlist[2 * i];
    double y = io.pointlist[2 * i + 1];
    points.emplace_back(x, y);
  }
  return points;
}

static std::vector<std::vector<TriangleIndex>>
getTriangleIndicesFromTriangulateio(const triangle::triangulateio &io) {
  std::vector<std::vector<TriangleIndex>> triangleIndices(
      static_cast<std::size_t>(io.numberofregions));
  for (int i = 0; i < io.numberoftriangles; ++i) {
    auto t0 = static_cast<std::size_t>(io.trianglelist[i * 3]);
    auto t1 = static_cast<std::size_t>(io.trianglelist[i * 3 + 1]);
    auto t2 = static_cast<std::size_t>(io.trianglelist[i * 3 + 2]);
    auto compIndex = static_cast<std::size_t>(io.triangleattributelist[i]) - 1;
    triangleIndices[compIndex].push_back({{t0, t1, t2}});
  }
  return triangleIndices;
}

static QPointF findCentroid(int triangleIndex, const int *triangles,
                            const double *points) {
  QPointF centroid(0, 0);
  for (int v = 0; v < 3; ++v) {
    int i = triangles[triangleIndex * 3 + v];
    double x = points[2 * i];
    double y = points[2 * i + 1];
    centroid += QPointF(x, y);
  }
  centroid /= 3.0;
  return centroid;
}

static bool appendUnassignedTriangleCentroids(const triangle::triangulateio &io,
                                              std::vector<QPointF> &holes) {
  bool foundUnassignedTriangles = false;
  for (int i = 0; i < io.numberoftriangles; ++i) {
    if (static_cast<int>(io.triangleattributelist[i]) == 0) {
      foundUnassignedTriangles = true;
      auto c = findCentroid(i, io.trianglelist, io.pointlist);
      holes.push_back(c);
      SPDLOG_DEBUG("  - adding hole at ({},{})", c.x(), c.y());
    }
  }
  return foundUnassignedTriangles;
}

// triangulate compartments without altering any boundary segments
void Triangulate::triangulateCompartments(
    const TriangulateBoundaries &boundaries) {
  // add a hole for each membrane
  std::vector<QPointF> holes;
  holes.reserve(rectangleIndices.size());
  for (const auto &rectangles : rectangleIndices) {
    QPointF centroid =
        findCentroid(rectangles.front(), boundaries.boundaryPoints);
    SPDLOG_DEBUG("Adding hole for membrane at ({},{})", centroid.x(),
                 centroid.y());
    holes.push_back(centroid);
  }
  auto in = toTriangulateio(boundaries);
  setHoleList(in, holes);
  triangle::triangulateio out;
  // call Triangle library with additional flags:
  //  - YY: disallow creation of Steiner points on segments
  //        (allowing these would invalidate the rectangle membrane indices)
  std::string triangleFlagsNoSteiner = std::string(triangleFlags).append("YY");
  triangle::triangulate(triangleFlagsNoSteiner.c_str(), &in, &out, nullptr);
  if (appendUnassignedTriangleCentroids(out, holes)) {
    // if there are triangles that are not assigned to a compartment,
    // insert a hole in the middle of each and re-mesh.
    setHoleList(in, holes);
    out.clear();
    triangle::triangulate(triangleFlagsNoSteiner.c_str(), &in, &out, nullptr);
  }
  points = getPointsFromTriangulateio(out);
  triangleIndices = getTriangleIndicesFromTriangulateio(out);
}

Triangulate::Triangulate(const TriangulateBoundaries &boundaries,
                         const TriangulateFixedPoints &fixedPoints) {
  auto newBoundaries = addMembranes(boundaries, fixedPoints);
  triangulateCompartments(newBoundaries);
}

const std::vector<QPointF> &Triangulate::getPoints() const { return points; }

const std::vector<std::vector<TriangleIndex>> &
Triangulate::getTriangleIndices() const {
  return triangleIndices;
}

const std::vector<std::vector<RectangleIndex>> &
Triangulate::getRectangleIndices() const {
  return rectangleIndices;
}

} // namespace mesh
