#include "triangle_wrapper.hpp"

#include <set>

#include <QDebug>

#include "logger.hpp"

namespace triangle_wrapper {

void Triangulate::setPointList(const std::vector<QPoint>& boundaryPoints) {
  in.pointlist =
      static_cast<double*>(malloc(2 * boundaryPoints.size() * sizeof(double)));
  in.numberofpoints = static_cast<int>(boundaryPoints.size());
  double* d = in.pointlist;
  for (const auto& p : boundaryPoints) {
    *d = static_cast<double>(p.x());
    ++d;
    *d = static_cast<double>(p.y());
    ++d;
  }
}

void Triangulate::setSegmentList(
    const std::vector<BoundarySegments>& boundaries) {
  std::size_t nSegments = 0;
  for (const auto& boundarySegments : boundaries) {
    nSegments += boundarySegments.size();
  }
  in.segmentlist = static_cast<int*>(malloc(2 * nSegments * sizeof(int)));
  in.numberofsegments = static_cast<int>(nSegments);
  in.segmentmarkerlist = static_cast<int*>(malloc(nSegments * sizeof(int)));
  int* seg = in.segmentlist;
  int* segm = in.segmentmarkerlist;
  int boundaryMarker = 2;
  for (const auto& boundarySegments : boundaries) {
    for (const auto& segmentIndexPair : boundarySegments) {
      *seg = static_cast<int>(segmentIndexPair[0]);
      ++seg;
      *seg = static_cast<int>(segmentIndexPair[1]);
      ++seg;
      *segm = boundaryMarker;
      ++segm;
    }
    ++boundaryMarker;
  }
}

void Triangulate::setRegionList(const std::vector<Compartment>& compartments) {
  if (!compartments.empty()) {
    in.regionlist =
        static_cast<double*>(malloc(4 * compartments.size() * sizeof(double)));
    in.numberofregions = static_cast<int>(compartments.size());
    double* r = in.regionlist;
    int i = 0;
    for (const auto& compartment : compartments) {
      *r = static_cast<double>(compartment.interiorPoint.x());
      ++r;
      *r = static_cast<double>(compartment.interiorPoint.y());
      ++r;
      *r = static_cast<double>(i);  // compartment index
      ++r;
      *r = compartment.maxTriangleArea;
      ++r;
      ++i;
    }
  }
}

void Triangulate::setHoleList(const std::vector<QPoint>& holes) {
  if (!holes.empty()) {
    in.holelist =
        static_cast<double*>(malloc(2 * holes.size() * sizeof(double)));
    in.numberofholes = static_cast<int>(holes.size());
    double* h = in.holelist;
    for (const auto& hole : holes) {
      *h = static_cast<double>(hole.x());
      ++h;
      *h = static_cast<double>(hole.y());
      ++h;
    }
  }
}

Triangulate::Triangulate(const std::vector<QPoint>& boundaryPoints,
                         const std::vector<BoundarySegments>& boundaries,
                         const std::vector<Compartment>& compartments,
                         const std::vector<QPoint>& holes) {
  // init Triangle library input data
  setPointList(boundaryPoints);
  setSegmentList(boundaries);
  setRegionList(compartments);
  setHoleList(holes);

  // call Triangle library
  //  - Q: no printf output
  //       (use V, VV, VVV for more verbose output)
  //  - z: 0-based indexing
  //  - p: supply Planar Straight Line Graph with vertices, segments, etc
  //  - j: remove unused vertices from output
  //  - q: generate quality triangles
  //       (add vertices such that triangle angles between 20-140 degrees)
  //  - A: regional attributes (interior point for each compartment)
  //  - a: max triangle area for each compartment
  triangle::triangulate("QzpjqAa", &in, &out, nullptr);

  // parse Triangle library output
  points.clear();
  for (int i = 0; i < out.numberofpoints; ++i) {
    double x = out.pointlist[2 * i];
    double y = out.pointlist[2 * i + 1];
    points.push_back(QPointF(x, y));
  }
  triangleIndices.clear();
  for (int i = 0; i < out.numberoftriangles; ++i) {
    auto t0 = static_cast<std::size_t>(out.trianglelist[i * 3]);
    auto t1 = static_cast<std::size_t>(out.trianglelist[i * 3 + 1]);
    auto t2 = static_cast<std::size_t>(out.trianglelist[i * 3 + 2]);
    auto compIndex = static_cast<std::size_t>(out.triangleattributelist[i]);
    triangleIndices.push_back({compIndex, t0, t1, t2});
  }
}

const std::vector<QPointF>& Triangulate::getPoints() const { return points; }

const std::vector<TriangleIndex>& Triangulate::getTriangleIndices() const {
  return triangleIndices;
}

}  // namespace triangle_wrapper
