#include "boundaries.hpp"
#include "boundary.hpp"
#include "contour_map.hpp"
#include "interior_point.hpp"
#include "mesh_utils.hpp"
#include "pixel_corner_iterator.hpp"
#include "sme/logger.hpp"
#include "sme/utils.hpp"
#include <algorithm>
#include <opencv2/imgproc.hpp>
#include <ranges>
#include <utility>

namespace sme::mesh {

static std::vector<QPoint>
toQPointsInvertYAxis(const std::vector<cv::Point> &points, int height) {
  std::vector<QPoint> v;
  v.reserve(points.size());
  for (const auto &p : points) {
    v.push_back({p.x, height - 1 - p.y});
  }
  return v;
}

static void
extractContoursFromMask(const cv::Mat &mask,
                        std::vector<std::vector<cv::Point>> &edges) {
  // get contours of compartment as closed loops
  std::vector<std::vector<cv::Point>> compContours;
  // for each contour, last component of hierarchy is index of parent
  std::vector<cv::Vec4i> hierarchy;
  cv::findContours(mask, compContours, hierarchy, cv::RETR_CCOMP,
                   cv::CHAIN_APPROX_NONE);
  for (std::size_t i = 0; i < compContours.size(); ++i) {
    auto &edgeContour = edges.emplace_back();
    const auto &compContour = compContours[i];
    bool outer = hierarchy[i][3] == -1; // -1: no parent, i.e. outer contour
    SPDLOG_TRACE("  - {} pixels", compContour.size());
    SPDLOG_TRACE("  - outer: {}", outer);
    PixelCornerIterator cpi(compContour, outer);
    while (!cpi.done()) {
      edgeContour.push_back(cpi.vertex());
      SPDLOG_TRACE("    - ({},{})", edgeContour.back().x, edgeContour.back().y);
      ++cpi;
    }
  }
}

static Contours getContours(const QImage &img,
                            const std::vector<QRgb> &compartmentColors) {
  Contours contours;
  for (auto col : compartmentColors) {
    auto binaryMask = makeBinaryMask(img, col);
    SPDLOG_TRACE("comp {:x}", col);
    extractContoursFromMask(binaryMask, contours.compartmentEdges);
  }
  auto binaryMask = makeBinaryMask(img, compartmentColors);
  SPDLOG_TRACE("domain");
  extractContoursFromMask(binaryMask, contours.domainEdges);
  return contours;
}

static std::vector<Boundary> splitContours(const QImage &img,
                                           Contours &contours) {
  std::vector<Boundary> boundaries;
  std::vector<std::vector<cv::Point>> loops;
  std::vector<std::vector<cv::Point>> lines;
  auto contourMap = ContourMap(img.size(), contours);
  for (auto &edges : contours.compartmentEdges) {
    // find the first fixed point, if any
    std::size_t startPixel{0};
    while (startPixel < edges.size() &&
           !contourMap.isFixedPoint(edges[startPixel])) {
      ++startPixel;
    }
    // no FP: closed loop
    if (startPixel == edges.size()) {
      SPDLOG_TRACE("Found loop with {} points", edges.size());
      if (std::ranges::none_of(loops, [&edges](const auto &l) {
            return common::isCyclicPermutation(edges, l);
          })) {
        SPDLOG_TRACE("  - adding loop", edges.size());
        auto points = toQPointsInvertYAxis(edges, img.height() + 1);
        boundaries.emplace_back(points, true);
        loops.push_back(std::move(edges));
      }
    } else {
      // need to split the loop into lines connecting fixed points
      // rotate so the first point is a fixed point
      std::ranges::rotate(
          edges,
          edges.begin() +
              static_cast<std::vector<cv::Point>::difference_type>(startPixel));
      std::vector<cv::Point> line;
      line.push_back(edges.front());
      for (std::size_t i = 1; i < edges.size(); ++i) {
        line.push_back(edges[i]);
        if (contourMap.isFixedPoint(edges[i])) {
          SPDLOG_TRACE("Finished line with {} points", line.size());
          if (std::ranges::none_of(lines, [&line](const auto &l) {
                return common::isCyclicPermutation(line, l);
              })) {
            SPDLOG_TRACE("  - adding line", edges.size());
            auto points = toQPointsInvertYAxis(line, img.height() + 1);
            boundaries.emplace_back(points, false);
            lines.push_back(std::move(line));
          }
          // start new line from final FP of previous line
          line.clear();
          line.push_back(edges[i]);
        }
      }
      line.push_back(edges.front());
      SPDLOG_TRACE("Finished line with {} points", line.size());
      if (std::ranges::none_of(lines, [&line](const auto &l) {
            return common::isCyclicPermutation(line, l);
          })) {
        SPDLOG_TRACE("  - adding line", edges.size());
        auto points = toQPointsInvertYAxis(line, img.height() + 1);
        boundaries.emplace_back(points, false);
        lines.push_back(std::move(line));
      }
    }
  }
  return boundaries;
}

Boundaries::Boundaries(const QImage &compartmentImage,
                       const std::vector<QRgb> &compartmentColors,
                       std::size_t simplifierType)
    : m_simplifierType{simplifierType} {
  auto edgeContours{getContours(compartmentImage, compartmentColors)};
  m_boundaries = splitContours(compartmentImage, edgeContours);
  m_polylineSimplifier.setBoundaries(m_boundaries);
}

Boundaries::~Boundaries() = default;

const std::vector<Boundary> &Boundaries::getBoundaries() const {
  return m_boundaries;
}

[[nodiscard]] std::size_t Boundaries::getSimplifierType() const {
  return m_simplifierType;
}

void Boundaries::setSimplifierType(std::size_t type) {
  m_simplifierType = type;
}

void Boundaries::setMaxPoints(std::size_t boundaryIndex,
                              std::size_t maxPoints) {
  if (m_simplifierType == 0) {
    m_boundaries[boundaryIndex].setMaxPoints(maxPoints);
  } else if (m_simplifierType == 1) {
    setMaxPoints(maxPoints);
  } else {
    SPDLOG_WARN("Invalid simplifierType: {}", m_simplifierType);
  }
}

void Boundaries::setMaxPoints(std::size_t maxPoints) {
  if (maxPoints == m_polylineSimplifier.getMaxPoints()) {
    return;
  }
  m_polylineSimplifier.setMaxPoints(m_boundaries, maxPoints);
}

std::size_t Boundaries::getMaxPoints(std::size_t boundaryIndex) const {
  if (m_simplifierType == 0) {
    return m_boundaries[boundaryIndex].getMaxPoints();
  } else if (m_simplifierType == 1) {
    return m_polylineSimplifier.getMaxPoints();
  }
  return 0;
}

std::vector<std::size_t> Boundaries::getMaxPoints() const {
  if (m_simplifierType == 0) {
    std::vector<std::size_t> v(m_boundaries.size());
    std::ranges::transform(m_boundaries, v.begin(),
                           [](const auto &b) { return b.getMaxPoints(); });
    return v;
  } else if (m_simplifierType == 1) {
    return {m_polylineSimplifier.getMaxPoints()};
  }
  return {};
}

void Boundaries::setMaxPoints() {
  if (m_simplifierType == 0) {
    for (auto &boundary : m_boundaries) {
      boundary.setMaxPoints();
    }
  } else {
    setMaxPoints(0);
  }
}

} // namespace sme::mesh
