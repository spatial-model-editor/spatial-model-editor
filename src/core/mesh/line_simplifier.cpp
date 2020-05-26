#include "line_simplifier.hpp"

#include <cmath>
#include <numeric>

#include "logger.hpp"

namespace mesh {

// triangle area in half-pixel units
static int triangleArea(const QPoint& a, const QPoint& b, const QPoint& c) {
  // https://en.wikipedia.org/wiki/Shoelace_formula
  return std::abs(a.x() * b.y() + b.x() * c.y() + c.x() * a.y() -
                  b.x() * a.y() - c.x() * b.y() - a.x() * c.y());
}

static std::vector<int> getTriangleAreas(const std::vector<QPoint>& v,
                                         bool isLoop) {
  // set all points to initially have infinite area (i.e. cannot be removed)
  std::vector<int> areas(v.size(), std::numeric_limits<int>::max());
  if (isLoop) {
    // first/last points can be removed for loops, so calculate areas
    areas.front() = triangleArea(v.back(), v.front(), v[1]);
    areas.back() = triangleArea(v[v.size() - 2], v.back(), v.front());
  }
  // calculate area for each intermediate point
  for (std::size_t i = 1; i < v.size() - 1; ++i) {
    areas[i] = triangleArea(v[i - 1], v[i], v[i + 1]);
  }
  return areas;
}

static inline std::size_t cyclicMod(std::size_t i, std::size_t size) {
  return i == size ? 0 : i;
}

static inline std::size_t cyclicIncrement(std::size_t i, std::size_t size) {
  return i == size - 1 ? 0 : i + 1;
}

static inline std::size_t cyclicDecrement(std::size_t i, std::size_t size) {
  return i == 0 ? size - 1 : i - 1;
}

static void updateTriangleAreas(std::vector<int>& areas,
                                std::size_t erasedIndex,
                                const std::vector<std::size_t>& ix,
                                const std::vector<QPoint>& pt) {
  // recalculate triangle areas for neighbouring points of removed point
  //  - note: if new area is smaller than previous area, use previous area
  // treat all boundaries as loops for simplicity of implementation
  // (non-loop start/end points have infinite initial area so are not altered)
  std::size_t sz = areas.size();
  auto i0 = cyclicMod(erasedIndex, sz);
  auto ip1 = cyclicIncrement(i0, sz);
  auto im1 = cyclicDecrement(i0, sz);
  auto im2 = cyclicDecrement(im1, sz);
  areas[i0] =
      std::max(areas[i0], triangleArea(pt[ix[im1]], pt[ix[i0]], pt[ix[ip1]]));
  areas[im1] =
      std::max(areas[im1], triangleArea(pt[ix[im2]], pt[ix[im1]], pt[ix[i0]]));
}

template <typename T>
static T pop(std::vector<T>& vec, std::size_t index) {
  using difference_type = typename std::vector<T>::difference_type;
  auto iter = vec.begin() + static_cast<difference_type>(index);
  T value = *iter;
  vec.erase(iter);
  return value;
}

static std::size_t popSmallestTriangleIndex(std::vector<std::size_t>& indices,
                                            std::vector<int>& areas,
                                            const std::vector<QPoint>& points) {
  auto iter = std::min_element(areas.cbegin(), areas.cend());
  auto i = static_cast<std::size_t>(std::distance(areas.cbegin(), iter));
  areas.erase(iter);
  std::size_t smallestTriangleIndex = pop(indices, i);
  updateTriangleAreas(areas, i, indices, points);
  return smallestTriangleIndex;
}

// get priority of each point in boundary
static std::vector<std::size_t> getPriorities(
    const std::vector<QPoint>& vertices, bool isLoop) {
  std::vector<std::size_t> priorities(vertices.size(), 0);
  std::size_t maxPoints = vertices.size();
  std::size_t minPoints = 2;
  if (isLoop) {
    minPoints = 3;
  }
  // start with all points, remove least important one-by-one
  std::vector<std::size_t> indices(maxPoints, 0);
  std::iota(indices.begin(), indices.end(), 0);
  auto areas = getTriangleAreas(vertices, isLoop);
  for (std::size_t priority = maxPoints; priority > minPoints; --priority) {
    auto index = popSmallestTriangleIndex(indices, areas, vertices);
    priorities[index] = priority;
  }
  // last minPoints points have 0 (i.e. maximum) priority
  return priorities;
}

// remove any points that have zero triangle area, i.e. that lie on a straight
// line such that removing them doesn't change the shape of the boundary
static std::vector<QPoint> removeDegenerateVertices(
    const std::vector<QPoint>& p, bool isLoop) {
  std::vector<QPoint> v;
  v.reserve(p.size());
  // first point can only be degenerate if boundary is a loop
  if (!isLoop || triangleArea(p.back(), p.front(), p[1]) != 0) {
    v.push_back(p.front());
    SPDLOG_TRACE(" - ({},{})", v.back().x(), v.back().y());
  }
  // intermediate points
  for (std::size_t i = 1; i < p.size() - 1; ++i) {
    if (triangleArea(p[i - 1], p[i], p[i + 1]) != 0) {
      v.push_back(p[i]);
      SPDLOG_TRACE(" - ({},{})", v.back().x(), v.back().y());
    }
  }
  // last point can only be degenerate if boundary is a loop
  if (!isLoop || triangleArea(p[p.size() - 2], p.back(), p.front()) != 0) {
    v.push_back(p.back());
    SPDLOG_TRACE(" - ({},{})", v.back().x(), v.back().y());
  }
  return v;
}

// distance between two points
static double distance(const QPoint& p0, const QPoint& p1) {
  return std::hypot(p0.x() - p1.x(), p0.y() - p1.y());
}

// shortest distance between line l0->l1 and point p
static double distanceFromLine(const QPoint& l0, const QPoint& l1,
                               const QPoint& p, double length) {
  // https://en.wikipedia.org/wiki/Distance_from_a_point_to_a_line
  return static_cast<double>(triangleArea(l0, l1, p)) / length;
}

LineError LineSimplifier::getLineError(const std::vector<QPoint>& line) const {
  LineError err{0, 0};
  int totalNumPixels = 0;
  QPoint pixel;        // a pixel on the original boundary
  QPoint deltaPixel;   // the change required to get to the next pixel
  std::size_t iv = 0;  // original boundary segment vertex index
  for (std::size_t il = 0; il < line.size() - 1; ++il) {
    // il: starting vertex of simplified boundary line segment
    SPDLOG_TRACE("line segment: ({},{})->({},{})", line[il].x(), line[il].y(),
                 line[il + 1].x(), line[il + 1].y());
    double lineLength = distance(line[il], line[il + 1]);
    while (vertices[iv] != line[il + 1]) {
      // sub-segment from original boundary
      pixel = vertices[iv];
      SPDLOG_TRACE("  - original segment ({},{})->({},{})", vertices[iv].x(),
                   vertices[iv].y(), vertices[iv + 1].x(),
                   vertices[iv + 1].y());
      deltaPixel = vertices[iv + 1] - pixel;
      auto numPixelsInSegment =
          std::max(std::abs(deltaPixel.x()), std::abs(deltaPixel.y()));
      deltaPixel /= numPixelsInSegment;
      for (int j = 0; j < numPixelsInSegment; ++j) {
        // distance of each pixel in original sub-segment from approx boundary
        double dist =
            distanceFromLine(line[il], line[il + 1], pixel, lineLength);
        err.total += dist;
        pixel += deltaPixel;
        ++totalNumPixels;
        SPDLOG_TRACE("    - pixel: ({},{}) : {}", pixel.x(), pixel.y(), dist);
      }
      ++iv;
    }
  }
  err.average = err.total / static_cast<double>(totalNumPixels);
  return err;
}

void LineSimplifier::getSimplifiedLine(std::vector<QPoint>& line,
                                       const LineError& allowedError) const {
  SPDLOG_DEBUG("Allowed error: total = {}, average = {}", allowedError.total,
               allowedError.average);
  for (std::size_t n = minNumPoints; n <= maxPoints(); ++n) {
    getSimplifiedLine(line, n);
    auto error = getLineError(line);
    SPDLOG_DEBUG("  - n = {} : total = {}, average = {}", n, error.total,
                 error.average);
    if (error.average <= allowedError.average ||
        error.total <= allowedError.total) {
      return;
    }
  }
}

void LineSimplifier::getSimplifiedLine(std::vector<QPoint>& line,
                                       std::size_t nPoints) const {
  line.clear();
  for (std::size_t i = 0; i < vertices.size(); ++i) {
    if (priorities[i] <= nPoints) {
      line.push_back(vertices[i]);
    }
  }
}

std::size_t LineSimplifier::maxPoints() const { return vertices.size(); }

LineSimplifier::LineSimplifier(const std::vector<QPoint>& points,
                               bool isClosedLoop)
    : vertices{removeDegenerateVertices(points, isClosedLoop)},
      minNumPoints{isClosedLoop ? std::size_t{3} : std::size_t{2}} {
  priorities = getPriorities(vertices, isClosedLoop);
}

}  // namespace mesh
