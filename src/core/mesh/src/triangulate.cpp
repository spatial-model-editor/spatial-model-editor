#include "triangulate.hpp"
#include "logger.hpp"
#include "triangle.hpp"
#include "utils.hpp"
#include <QImage>
#include <QPainter>
#include <QString>
#include <initializer_list>

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

static void
setSegmentList(triangle::triangulateio &in,
               const std::vector<TriangulateBoundarySegments> &boundaries) {
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
  int *segmarker = in.segmentmarkerlist;
  // 2-based indexing of boundaries
  // 0 and 1 may be used by triangle library
  int boundaryMarker = 2;
  for (const auto &boundary : boundaries) {
    for (const auto &segment : boundary) {
      *seg = static_cast<int>(segment.start);
      ++seg;
      *seg = static_cast<int>(segment.end);
      ++seg;
      *segmarker = boundaryMarker;
      ++segmarker;
    }
    ++boundaryMarker;
  }
}

static void
setRegionList(triangle::triangulateio &in,
              const std::vector<TriangulateCompartment> &compartments) {
  // 1-based indexing of compartments
  // 0 is then used for triangles that are not part of a compartment
  free(in.regionlist);
  in.regionlist = nullptr;
  if (compartments.empty()) {
    return;
  }
  std::size_t nRegions{0};
  for (const auto &compartment : compartments) {
    nRegions += compartment.interiorPoints.size();
  }
  in.regionlist = static_cast<double *>(malloc(4 * nRegions * sizeof(double)));
  in.numberofregions = static_cast<int>(nRegions);
  double *r = in.regionlist;
  int i = 1;
  for (const auto &compartment : compartments) {
    for (const auto &point : compartment.interiorPoints) {
      *r = static_cast<double>(point.x());
      ++r;
      *r = static_cast<double>(point.y());
      ++r;
      *r = static_cast<double>(i); // compartment index + 1
      ++r;
      *r = compartment.maxTriangleArea;
      ++r;
    }
    ++i;
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
  setPointList(io, tid.vertices);
  setSegmentList(io, tid.boundaries);
  setRegionList(io, tid.compartments);
  return io;
}

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

static std::vector<std::vector<TriangulateTriangleIndex>>
getTriangleIndicesFromTriangulateio(const triangle::triangulateio &io) {
  std::vector<std::vector<TriangulateTriangleIndex>> triangleIndices(
      static_cast<std::size_t>(io.numberofregions));
  for (int i = 0; i < io.numberoftriangles; ++i) {
    auto t0 = static_cast<std::size_t>(io.trianglelist[i * 3]);
    auto t1 = static_cast<std::size_t>(io.trianglelist[i * 3 + 1]);
    auto t2 = static_cast<std::size_t>(io.trianglelist[i * 3 + 2]);
    auto compIndex = static_cast<std::size_t>(io.triangleattributelist[i]) - 1;
    triangleIndices[compIndex].push_back({{t0, t1, t2}});
  }
  while (!triangleIndices.empty() && triangleIndices.back().empty()) {
    // number of regions may be larger than the number of compartments
    // if multiple regions have the same compartment index
    triangleIndices.pop_back();
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

Triangulate::Triangulate(const TriangulateBoundaries &boundaries) {
  std::vector<QPointF> holes;
  auto in = toTriangulateio(boundaries);
  setHoleList(in, holes);
  triangle::triangulateio out;
  triangle::triangulate(triangleFlags, &in, &out, nullptr);
  if (appendUnassignedTriangleCentroids(out, holes)) {
    // if there are triangles that are not assigned to a compartment,
    // insert a hole in the middle of each and re-mesh.
    setHoleList(in, holes);
    out.clear();
    triangle::triangulate(triangleFlags, &in, &out, nullptr);
  }
  points = getPointsFromTriangulateio(out);
  triangleIndices = getTriangleIndicesFromTriangulateio(out);
}

const std::vector<QPointF> &Triangulate::getPoints() const { return points; }

const std::vector<std::vector<TriangulateTriangleIndex>> &
Triangulate::getTriangleIndices() const {
  return triangleIndices;
}

} // namespace mesh
