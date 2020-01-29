#include "triangulate.hpp"

#include "logger.hpp"
#include "triangle/triangle.hpp"

namespace triangulate {

void Triangulate::setPointList(
    triangle::triangulateio& in,
    const std::vector<QPointF>& boundaryPoints) const {
  free(in.pointlist);
  in.pointlist = nullptr;
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
    triangle::triangulateio& in,
    const std::vector<BoundarySegments>& boundaries) const {
  free(in.segmentlist);
  in.segmentlist = nullptr;
  free(in.segmentmarkerlist);
  in.segmentmarkerlist = nullptr;
  std::size_t nSegments = 0;
  for (const auto& boundarySegments : boundaries) {
    nSegments += boundarySegments.size();
  }
  in.segmentlist = static_cast<int*>(malloc(2 * nSegments * sizeof(int)));
  in.numberofsegments = static_cast<int>(nSegments);
  in.segmentmarkerlist = static_cast<int*>(malloc(nSegments * sizeof(int)));
  int* seg = in.segmentlist;
  int* segm = in.segmentmarkerlist;
  // 2-based indexing of boundaries
  // 0 and 1 may be used by triangle library
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

void Triangulate::setRegionList(
    triangle::triangulateio& in,
    const std::vector<Compartment>& compartments) const {
  // 1-based indexing of compartments
  // 0 is then used for triangles that are not part of a compartment
  free(in.regionlist);
  in.regionlist = nullptr;
  if (!compartments.empty()) {
    in.regionlist =
        static_cast<double*>(malloc(4 * compartments.size() * sizeof(double)));
    in.numberofregions = static_cast<int>(compartments.size());
    double* r = in.regionlist;
    int i = 1;
    for (const auto& compartment : compartments) {
      *r = static_cast<double>(compartment.interiorPoint.x());
      ++r;
      *r = static_cast<double>(compartment.interiorPoint.y());
      ++r;
      *r = static_cast<double>(i);  // compartment index + 1
      ++r;
      *r = compartment.maxTriangleArea;
      ++r;
      ++i;
    }
  }
}

void Triangulate::setHoleList(triangle::triangulateio& in,
                              const std::vector<QPointF>& holes) const {
  free(in.holelist);
  in.holelist = nullptr;
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

// construct a normal unit vector to the line between start and end
static QPointF unitNormalVector(const QPointF start, const QPointF end) {
  QPointF delta = end - start;
  auto normal = QPointF(-delta.y(), delta.x());
  double length = sqrt(QPointF::dotProduct(normal, normal));
  return normal / length;
}

// ensure segments all have the same orientation
// O(N^2) in number of segments
static void orientSegments(BoundarySegments& segments) {
  if (segments.size() < 2) {
    return;
  }
  std::size_t prev = 0;
  std::size_t rhs = segments[prev][1];
  for (std::size_t i = 0; i < segments.size() - 1; ++i) {
    for (std::size_t j = 0; j < segments.size(); ++j) {
      if (segments[j][0] == rhs && j != prev) {
        prev = j;
        rhs = segments[prev][1];
        break;
      }
      if (segments[j][1] == rhs && j != prev) {
        std::swap(segments[j][0], segments[j][1]);
        prev = j;
        rhs = segments[prev][1];
        break;
      }
    }
  }
}

triangle::triangulateio Triangulate::triangulateBoundaries(
    const std::vector<QPointF>& boundaryPoints,
    const std::vector<BoundarySegments>& boundaries,
    const std::vector<Compartment>& compartments,
    const std::vector<Membrane>& membranes) {
  // triangulate boundary points
  triangle::triangulateio in;
  setPointList(in, boundaryPoints);
  setSegmentList(in, boundaries);
  setRegionList(in, compartments);
  triangle::triangulateio out;
  triangle::triangulate(triangleFlags.c_str(), &in, &out, nullptr);
  SPDLOG_DEBUG("Initial triangulation: {} segments, {} points",
               out.numberofsegments, out.numberofpoints);

  // remove non-boundary points & re-index
  std::vector<QPointF> newPoints;
  std::vector<BoundarySegments> newBoundaries(boundaries.size(),
                                              BoundarySegments{});
  constexpr std::size_t nullIndex = std::numeric_limits<std::size_t>::max();
  std::vector<std::size_t> pointIndex(
      static_cast<std::size_t>(out.numberofpoints), nullIndex);
  std::size_t currentIndex = 0;
  for (int iSeg = 0; iSeg < out.numberofsegments; ++iSeg) {
    auto start = static_cast<std::size_t>(out.segmentlist[2 * iSeg]);
    auto end = static_cast<std::size_t>(out.segmentlist[2 * iSeg + 1]);
    auto boundary = static_cast<std::size_t>(out.segmentmarkerlist[iSeg] - 2);
    // add points
    for (auto iPoint : {start, end}) {
      if (pointIndex[iPoint] == nullIndex) {
        pointIndex[iPoint] = currentIndex;
        ++currentIndex;
        newPoints.push_back(
            QPointF(out.pointlist[2 * iPoint], out.pointlist[2 * iPoint + 1]));
      }
    }
    // add segment
    newBoundaries[boundary].push_back({{pointIndex[start], pointIndex[end]}});
  }
  SPDLOG_DEBUG("After re-indexing:");
  SPDLOG_DEBUG("  - {} points", newPoints.size());
  for (const auto& s : newBoundaries) {
    SPDLOG_DEBUG("  - {} segment boundary", s.size());
  }

  // for each membrane segment, add corresponding
  //  - outer points, if not already present
  //  - outer membrane segment using these points
  //  - rectangle index using the old & new membrane segments
  std::vector<std::size_t> matchingPointIndex(newPoints.size(), nullIndex);
  for (const auto& membrane : membranes) {
    orientSegments(newBoundaries[membrane.boundaryIndex]);
    auto& outer = newBoundaries.emplace_back();
    const auto& inner = newBoundaries[membrane.boundaryIndex];
    SPDLOG_DEBUG(
        "  - adding outer membrane around {}-point boundary[{}], width {}",
        inner.size(), membrane.boundaryIndex, membrane.width);
    auto& rect = rectangleIndices.emplace_back();
    for (const auto& segmentPair : inner) {
      auto start = segmentPair[0];
      auto end = segmentPair[1];
      auto normal =
          unitNormalVector(newPoints[start], newPoints[end]) * membrane.width;
      for (auto iPoint : segmentPair) {
        if (auto& mpi = matchingPointIndex[iPoint]; mpi == nullIndex) {
          // add outer point
          mpi = currentIndex;
          ++currentIndex;
          newPoints.emplace_back(newPoints[iPoint] + normal);
        } else {
          // already have outer point from adjacent segment: calculate where it
          // should be using this segment, then move existing point to average
          // of the two calculated locations
          newPoints[mpi] = 0.5 * (newPoints[mpi] + newPoints[iPoint] + normal);
        }
      }
      // add segment to outer membrane
      outer.push_back({{matchingPointIndex[start], matchingPointIndex[end]}});
      // add rectangle
      rect.push_back(
          {{start, end, matchingPointIndex[end], matchingPointIndex[start]}});
    }
  }
  setPointList(in, newPoints);
  setSegmentList(in, newBoundaries);

  SPDLOG_DEBUG("After constructing membranes:");
  SPDLOG_DEBUG("  - {} points", newPoints.size());
  for (const auto& s : newBoundaries) {
    SPDLOG_DEBUG("  - {} segment boundary", s.size());
  }

  // add a hole for each membrane
  std::vector<QPointF> holes;
  holes.reserve(rectangleIndices.size());
  for (const auto& membraneRectangleIndices : rectangleIndices) {
    QPointF centroid(0, 0);
    for (auto i : membraneRectangleIndices[0]) {
      centroid += newPoints[i];
    }
    centroid /= 4.0;
    SPDLOG_DEBUG("Adding hole for membrane at ({},{})", centroid.x(),
                 centroid.y());
    holes.push_back(centroid);
  }
  setHoleList(in, holes);

  return in;
}

void Triangulate::triangulateCompartments(triangle::triangulateio& in) {
  triangle::triangulateio out;
  // get existing holes
  std::vector<QPointF> holes;
  if (in.holelist != nullptr) {
    for (int i = 0; i < in.numberofholes; ++i) {
      holes.push_back(QPointF(in.holelist[2 * i], in.holelist[2 * i + 1]));
    }
  }
  bool allTrianglesAssigned = false;
  while (!allTrianglesAssigned) {
    // call Triangle library
    //  - YY: disallow creation of Steiner points on segments
    triangle::triangulate((triangleFlags + "YY").c_str(), &in, &out, nullptr);
    allTrianglesAssigned = true;

    // if there are triangles with 0 as regional attribute,
    // insert a hole in the middle of the first such triangle and re-mesh.
    // repeat until all triangles are assigned to a compartment (i.e. have a non
    // zero regional attribute)
    for (int i = 0; i < out.numberoftriangles; ++i) {
      if (static_cast<int>(out.triangleattributelist[i]) == 0) {
        allTrianglesAssigned = false;
        QPointF centroid(0, 0);
        for (int v = 0; v < 3; ++v) {
          int tIndex = out.trianglelist[i * 3 + v];
          double x = out.pointlist[2 * tIndex];
          double y = out.pointlist[2 * tIndex + 1];
          centroid += QPointF(x, y);
        }
        centroid /= 3.0;
        out.clear();
        holes.push_back(centroid);
        SPDLOG_DEBUG("Triangle {} is not part of a compartment", i);
        SPDLOG_DEBUG("  - adding hole at ({},{}) and re-triangulating",
                     centroid.x(), centroid.y());
        setHoleList(in, holes);
        break;
      }
    }
  }

  // parse Triangle library output
  points.clear();
  points.reserve(static_cast<std::size_t>(out.numberofpoints));
  for (int i = 0; i < out.numberofpoints; ++i) {
    double x = out.pointlist[2 * i];
    double y = out.pointlist[2 * i + 1];
    points.emplace_back(x, y);
  }
  for (int i = 0; i < out.numberoftriangles; ++i) {
    auto t0 = static_cast<std::size_t>(out.trianglelist[i * 3]);
    auto t1 = static_cast<std::size_t>(out.trianglelist[i * 3 + 1]);
    auto t2 = static_cast<std::size_t>(out.trianglelist[i * 3 + 2]);
    auto compIndex = static_cast<std::size_t>(out.triangleattributelist[i]) - 1;
    triangleIndices[compIndex].push_back({{t0, t1, t2}});
  }
}

Triangulate::Triangulate(const std::vector<QPointF>& boundaryPoints,
                         const std::vector<BoundarySegments>& boundaries,
                         const std::vector<Compartment>& compartments,
                         const std::vector<Membrane>& membranes)
    : triangleIndices(compartments.size(), std::vector<TriangleIndex>()) {
  auto in = triangulateBoundaries(boundaryPoints, boundaries, compartments,
                                  membranes);
  triangulateCompartments(in);
}

const std::vector<QPointF>& Triangulate::getPoints() const { return points; }

const std::vector<std::vector<TriangleIndex>>& Triangulate::getTriangleIndices()
    const {
  return triangleIndices;
}

const std::vector<std::vector<RectangleIndex>>&
Triangulate::getRectangleIndices() const {
  return rectangleIndices;
}

}  // namespace triangulate
