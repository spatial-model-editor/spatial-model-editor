#include "contours.hpp"
#include "contour_map.hpp"
#include "logger.hpp"
#include "mesh_types.hpp"
#include "utils.hpp"
#include <QColor>
#include <QPoint>
#include <QSize>
#include <algorithm>
#include <cstddef>
#include <limits>
#include <memory>
#include <opencv2/core/cvdef.h>
#include <opencv2/core/hal/interface.h>
#include <opencv2/core/matx.hpp>
#include <opencv2/imgproc.hpp>
#include <stdint.h>

namespace mesh {

static constexpr std::size_t nullIndex{std::numeric_limits<std::size_t>::max()};

static std::string
getMembraneName(QRgb col1, QRgb col2,
                const std::vector<std::pair<std::string, ColourPair>>
                    &membraneColourPairs) {
  for (const auto &[name, colPair] : membraneColourPairs) {
    auto [c1, c2] = colPair;
    if ((col1 == c1 && col2 == c2) || (col1 == c2 && col2 == c1)) {
      return name;
    }
  }
  return {};
}

static std::string
getMembraneName(const cv::Point &p1, const cv::Point &p2, const QImage &img,
                const std::vector<std::pair<std::string, ColourPair>>
                    &membraneColourPairs) {
  QRgb col1 = img.pixel(p1.x, p1.y);
  QRgb col2 = img.pixel(p2.x, p2.y);
  return getMembraneName(col1, col2, membraneColourPairs);
}

static std::optional<std::size_t>
getMembraneIndex(const std::string &id,
                 const std::vector<std::pair<std::string, ColourPair>>
                     &membraneColourPairs) {
  for (std::size_t i = 0; i < membraneColourPairs.size(); ++i) {
    if (id == membraneColourPairs[i].first) {
      return i;
    }
  }
  SPDLOG_WARN("Membrane '{}' not found", id);
  return {};
}

static std::vector<QPoint>
toQPointsInvertYAxis(const std::vector<cv::Point> &points, int height) {
  std::vector<QPoint> v;
  v.reserve(points.size());
  for (const auto &p : points) {
    v.push_back({p.x, height - 1 - p.y});
  }
  return v;
}

static cv::Mat makeBinaryMask(const QImage &img, QRgb col) {
  cv::Mat m(img.height(), img.width(), CV_8UC1, cv::Scalar(0));
  for (int x = 0; x < img.width(); ++x) {
    for (int y = 0; y < img.height(); ++y) {
      if (img.pixel(x, y) == col) {
        m.at<uint8_t>(y, x) = 255;
      }
    }
  }
  return m;
}

static std::vector<std::vector<cv::Point>>
getContours(const QImage &img, const std::vector<QRgb> &compartmentColours) {
  std::vector<std::vector<cv::Point>> contours;
  for (auto col : compartmentColours) {
    auto binaryMask = makeBinaryMask(img, col);
    // get contours of compartment as closed loops
    std::vector<std::vector<cv::Point>> compContours;
    std::vector<cv::Vec4i> hierarchy;
    cv::findContours(binaryMask, compContours, hierarchy, cv::RETR_LIST,
                     cv::CHAIN_APPROX_NONE);
    SPDLOG_TRACE("comp {:x}", col);
    for (auto &compContour : compContours) {
      SPDLOG_TRACE("  - {}", compContour.size());
      contours.push_back(std::move(compContour));
    }
  }
  return contours;
}

static double avgDistanceBetweenContours(const std::vector<cv::Point> &c1,
                                         const std::vector<cv::Point> &c2) {
  // todo: consider counting adjacent pixels instead, should be more reliable
  // albeit probably more expensive
  std::size_t min = std::min(c1.size(), c2.size());
  std::size_t max = std::max(c1.size(), c2.size());
  double extraPixels = static_cast<double>(max - min);
  double forwardsNorm{0};
  for (std::size_t i = 0; i < min; ++i) {
    forwardsNorm += cv::norm(c1[i] - c2[i]);
  }
  // use distance between last matching pair for any extra pixels
  forwardsNorm += extraPixels * cv::norm(c1[min - 1] - c2[min - 1]);
  double backwardsNorm{0};
  for (std::size_t i = 0; i < min; ++i) {
    backwardsNorm += cv::norm(c1[c1.size() - 1 - i] - c2[i]);
  }
  backwardsNorm += extraPixels * cv::norm(c1[c1.size() - min] - c2[min - 1]);
  return std::min(forwardsNorm, backwardsNorm) / static_cast<double>(max);
}

static std::vector<Boundary> extractClosedLoopBoundaries(
    const QImage &img,
    const std::vector<std::pair<std::string, ColourPair>> &membraneColourPairs,
    std::vector<std::vector<cv::Point>> &contours,
    QImage &boundaryPixelsImage) {
  std::vector<Boundary> boundaries;
  auto contourMap = ContourMap(img.size(), contours);
  std::size_t colourIndex{0};
  for (std::size_t contourIndex = 0; contourIndex < contours.size();
       ++contourIndex) {
    auto &contour = contours[contourIndex];
    if (!contour.empty()) {
      bool isLoop{false};
      bool isMembrane{false};
      std::string membraneName{};
      std::size_t adjacentContourIndex =
          contourMap.getAdjacentContourIndex(contour[0]);
      SPDLOG_TRACE("contour {} : adjacent contour {}", contourIndex,
                   adjacentContourIndex);
      if (contourMap.hasNeighbourWithThisIndex(contour, adjacentContourIndex) &&
          (adjacentContourIndex == nullIndex ||
           contourMap.hasNeighbourWithThisIndex(contours[adjacentContourIndex],
                                                contourIndex))) {
        isLoop = true;
        SPDLOG_TRACE("contour {} is a loop", contourIndex);
        SPDLOG_TRACE("  - removing contour {}", contourIndex);
        if (adjacentContourIndex != nullIndex) {
          isMembrane = true;
          SPDLOG_TRACE("  - forms membrane with contour {}",
                       adjacentContourIndex);
          auto &adjacentContour = contours[adjacentContourIndex];
          membraneName = getMembraneName(contour[0], adjacentContour[0], img,
                                         membraneColourPairs);
          SPDLOG_TRACE("  - membrane name '{}'", membraneName);
          SPDLOG_TRACE("  - removing contour {}", adjacentContourIndex);
          adjacentContour.clear();
        }
      }
      if (isLoop) {
        auto c = utils::indexedColours()[colourIndex].rgb();
        ++colourIndex;
        for (const auto &p : contour) {
          boundaryPixelsImage.setPixel(p.x, p.y, c);
        }
        auto points = toQPointsInvertYAxis(contour, img.height());
        contour.clear();
        auto membraneIndex =
            getMembraneIndex(membraneName, membraneColourPairs).value_or(0);
        SPDLOG_TRACE("  - membrane index '{}'", membraneIndex);
        auto boundary =
            Boundary(points, isLoop, isMembrane, membraneName, membraneIndex);
        if (boundary.isValid()) {
          boundaries.push_back(std::move(boundary));
        } else {
          SPDLOG_TRACE("  -> ignoring invalid boundary loop");
        }
      }
    }
  }
  contours.erase(std::remove_if(contours.begin(), contours.end(),
                                [](auto &x) { return x.empty(); }),
                 contours.end());
  return boundaries;
}

namespace {
struct ContourLine {
  std::vector<cv::Point> points;
  std::size_t contourIndex;
  std::size_t adjacentContourIndex;
  std::string membraneName;
  std::size_t membraneIndex;
};
} // namespace

static ContourLine extractLineFromContour(std::vector<cv::Point> &contour,
                                          const ContourMap &contourMap,
                                          bool cyclic = false) {
  ContourLine line;
  line.points.reserve(contour.size());
  line.contourIndex = contourMap.getContourIndex(contour[0]);
  line.adjacentContourIndex = contourMap.getAdjacentContourIndex(contour[0]);
  SPDLOG_TRACE("contour {}", line.contourIndex);
  SPDLOG_TRACE("  - initial number of pixels {}", contour.size());
  SPDLOG_TRACE("  - starting adjacent contour index {}",
               line.adjacentContourIndex);
  std::size_t startPixel{0};
  std::size_t endPixel{1};
  while (endPixel + 1 < contour.size() &&
         contourMap.hasNeighbourWithThisIndex(contour[endPixel],
                                              line.adjacentContourIndex)) {
    ++endPixel;
  }
  SPDLOG_TRACE("  - end pixel {} ({},{})", endPixel, contour[endPixel].x,
               contour[endPixel].y);
  if (cyclic && contourMap.hasNeighbourWithThisIndex(
                    contour.back(), line.adjacentContourIndex)) {
    startPixel = contour.size() - 1;
    while (startPixel > 0 &&
           contourMap.hasNeighbourWithThisIndex(contour[startPixel],
                                                line.adjacentContourIndex)) {
      --startPixel;
    }
  }
  SPDLOG_TRACE("  - start pixel {} ({},{})", startPixel, contour[startPixel].x,
               contour[startPixel].y);
  // extract line
  if (startPixel != 0) {
    for (std::size_t i = startPixel; i < contour.size(); ++i) {
      line.points.push_back(contour[i]);
    }
  }
  for (std::size_t i = 0; i < endPixel; ++i) {
    line.points.push_back(contour[i]);
  }
  // remove these points from contour
  utils::cyclicErase(contour, startPixel, endPixel);
  return line;
}

static std::vector<ContourLine>
extractLinesFromContours(const QImage &img,
                         std::vector<std::vector<cv::Point>> &contours) {
  std::vector<ContourLine> lines;
  auto contourMap = ContourMap(img.size(), contours);
  for (auto &contour : contours) {
    // first line can be cyclic, as original contour is a loop
    lines.push_back(extractLineFromContour(contour, contourMap, true));
    while (contour.size() > 1) {
      // subsequent lines cannot be cyclic, contour no longer a loop
      // ignore any leftover single pixels - typically artefacts of splitting
      lines.push_back(extractLineFromContour(contour, contourMap));
    }
  }
  return lines;
}

static void removeAdjacentMembraneLines(
    std::vector<ContourLine> &lines, const QImage &img,
    const std::vector<std::pair<std::string, ColourPair>>
        &membraneColourPairs) {
  for (std::size_t i = 0; i < lines.size(); ++i) {
    auto &line = lines[i];
    SPDLOG_TRACE("line {}:", i);
    SPDLOG_TRACE("  - contour {}", line.contourIndex);
    SPDLOG_TRACE("  - adjacent contour {}", line.adjacentContourIndex);
    if (line.adjacentContourIndex == nullIndex) {
      SPDLOG_TRACE("  -> line has no adjacent contour: skipping");
    } else {
      // find closest line with correct contour indices
      double minDistance{std::numeric_limits<double>::max()};
      std::size_t closestLine{nullIndex};
      for (std::size_t j = i + 1; j < lines.size(); ++j) {
        if (lines[j].adjacentContourIndex == line.contourIndex &&
            lines[j].contourIndex == line.adjacentContourIndex) {
          auto dist = avgDistanceBetweenContours(line.points, lines[j].points);
          if (dist < minDistance) {
            minDistance = dist;
            closestLine = j;
          }
          SPDLOG_TRACE("  -> line {} dist: {}", j, dist);
        }
      }
      if (closestLine == nullIndex) {
        SPDLOG_ERROR("Failed to find adjacent membrane line");
      } else {
        line.membraneName =
            getMembraneName(line.points[0], lines[closestLine].points[0], img,
                            membraneColourPairs);
        line.membraneIndex =
            getMembraneIndex(line.membraneName, membraneColourPairs)
                .value_or(0);
        SPDLOG_TRACE("  - membrane name '{}'", line.membraneName);
        SPDLOG_TRACE("  -> removing adjacent line {}", closestLine);
        lines.erase(lines.begin() +
                    static_cast<std::vector<ContourLine>::difference_type>(
                        closestLine));
      }
    }
  }
}

static void connectNewEndPoint(std::vector<cv::Point> &points,
                               cv::Point newPoint, bool connectToStart,
                               const cv::Mat &cvImg) {
  if (connectToStart) {
    std::reverse(points.begin(), points.end());
  }
  SPDLOG_TRACE("old end point ({},{})", points.back().x, points.back().y);
  if (points.size() > 1) {
    points.pop_back();
  }
  SPDLOG_TRACE("connect ({},{}) to ({},{})", points.back().x, points.back().y,
               newPoint.x, newPoint.y);
  cv::LineIterator li(cvImg, points.back(), newPoint);
  ++li; // skip first pixel in line: already present
  for (int k = 0; k < li.count - 1; ++k) {
    SPDLOG_TRACE("  - adding ({},{})", li.pos().x, li.pos().y);
    points.push_back(li.pos());
    ++li;
  }
  if (connectToStart) {
    std::reverse(points.begin(), points.end());
  }
}

static std::vector<cv::Point> drawStraightLine(cv::Point startPoint,
                                               cv::Point endPoint,
                                               const cv::Mat &cvImg) {
  std::vector<cv::Point> points;
  SPDLOG_TRACE("line from ({},{}) to ({},{})", startPoint.x, startPoint.y,
               endPoint.x, endPoint.y);
  cv::LineIterator li(cvImg, startPoint, endPoint);
  points.reserve(static_cast<std::size_t>(li.count));
  for (int k = 0; k < li.count; ++k) {
    SPDLOG_TRACE("  - adding ({},{})", li.pos().x, li.pos().y);
    points.push_back(li.pos());
    ++li;
  }
  return points;
}

static std::vector<double>
getDistanceToOtherPoints(std::size_t i0, const cv::Point &point,
                         const std::vector<ContourLine> &lines,
                         const std::vector<bool> &endPointIsMerged) {
  std::vector<double> dist(2 * lines.size(),
                           std::numeric_limits<double>::max());
  for (std::size_t j = 0; j < dist.size(); ++j) {
    if (!endPointIsMerged[j] && j / 2 != i0 / 2) {
      if (j % 2 == 0) {
        dist[j] = cv::norm(lines[j / 2].points.front() - point);
      } else {
        dist[j] = cv::norm(lines[j / 2].points.back() - point);
      }
      SPDLOG_DEBUG("  - line {}, front:{}, dist {}", j / 2, j % 2 == 0, dist[j]);
    }
  }
  return dist;
}

static void mergeEndPointTriplets(std::vector<ContourLine> &lines,
                                  const cv::Mat &cvImg) {
  if (2 * lines.size() % 3 != 0) {
    SPDLOG_WARN("Number of endpoints {} is not a multiple of 3",
                2 * lines.size());
    // find contour with end points that are closest to each other
    //  - most likely candidate for a single pixel line that was valid but
    //  discarded as an artefact
    //  - add a line between these two points
    //  - hope for the best...
    cv::Point pStart;
    cv::Point pEnd;
    double minDist{std::numeric_limits<double>::max()};
    for (const auto &line : lines) {
      double d{cv::norm(line.points.front() - line.points.back())};
      if (d < minDist) {
        minDist = d;
        pStart = line.points.front();
        pEnd = line.points.back();
      }
    }
    ContourLine cl;
    SPDLOG_DEBUG("  - adding line from ({},{}) to ({},{})", pStart.x, pStart.y,
                pEnd.x, pEnd.y);
    cl.points = drawStraightLine(pStart, pEnd, cvImg);
    lines.push_back(std::move(cl));
  }
  // identify & merge endpoints
  std::vector<bool> endPointIsMerged(2 * lines.size(), false);
  std::size_t nEndPointsToMerge{2 * lines.size() / 3};
  SPDLOG_TRACE("end point triplets to merge: {}", nEndPointsToMerge);
  for (std::size_t i = 0; i < nEndPointsToMerge; ++i) {
    // find next endpoint to merge
    std::size_t i0{0};
    while (endPointIsMerged[i0]) {
      ++i0;
    }
    auto point = lines[i0 / 2].points.front();
    if (i0 % 2 != 0) {
      point = lines[i0 / 2].points.back();
    }
    SPDLOG_DEBUG("line {}, end point ({},{}), front: {}", i0 / 2, point.x,
                point.y, i0 % 2 == 0);
    auto dist = getDistanceToOtherPoints(i0, point, lines, endPointIsMerged);
    // closest two end points:
    auto i1 = utils::min_element_index(dist);
    SPDLOG_DEBUG("    -> closest point: line {} front: {} (dist: {})", i1 / 2,
                i1 % 2 == 0, dist[i1]);
    dist[i1] = std::numeric_limits<double>::max();
    auto i2 = utils::min_element_index(dist);
    SPDLOG_DEBUG("    -> next-closest point: line {} front: {} (dist: {})",
                i2 / 2, i2 % 2 == 0, dist[i2]);
    connectNewEndPoint(lines[i1 / 2].points, point, i1 % 2 == 0, cvImg);
    connectNewEndPoint(lines[i2 / 2].points, point, i2 % 2 == 0, cvImg);
    endPointIsMerged[i0] = true;
    endPointIsMerged[i1] = true;
    endPointIsMerged[i2] = true;
  }
}

static std::size_t getOrInsertFPIndex(const QPoint &p,
                                      std::vector<FixedPoint> &fps) {
  // return index of item in points that matches p
  SPDLOG_TRACE("looking for point ({}, {})", p.x(), p.y());
  for (std::size_t i = 0; i < fps.size(); ++i) {
    if (fps[i].point == p) {
      SPDLOG_TRACE("  -> found [{}] : ({}, {})", i, fps[i].point.x(),
                   fps[i].point.y());
      return i;
    }
  }
  // if not found: add p to vector and return its index
  SPDLOG_TRACE("  -> added new point");
  auto &newFP = fps.emplace_back();
  newFP.point = p;
  return fps.size() - 1;
}

static void saveContoursImageForDebugging(
    const std::string &filename, const QSize &imgSize,
    const std::vector<std::vector<cv::Point>> &lines = {},
    const std::vector<Boundary> &boundaries = {}) {
  cv::Mat cvLinesImage =
      cv::Mat::zeros(imgSize.height(), imgSize.width(), CV_8UC3);
  for (size_t i = 0; i < boundaries.size(); i++) {
    auto c = utils::indexedColours()[i].rgb();
    cv::Scalar color = cv::Scalar(qRed(c), qGreen(c), qBlue(c));
    std::vector<cv::Point> cvLine;
    cvLine.reserve(boundaries[i].getPoints().size());
    for (const auto &p : boundaries[i].getPoints()) {
      cvLine.push_back({p.x(), imgSize.height() - 1 - p.y()});
    }
    cv::polylines(cvLinesImage, cvLine, true, color);
  }
  for (size_t i = 0; i < lines.size(); i++) {
    auto c = utils::indexedColours()[i + boundaries.size()].rgb();
    cv::Scalar color = cv::Scalar(qRed(c), qGreen(c), qBlue(c));
    cv::polylines(cvLinesImage, lines[i], false, color);
  }
  auto qImg =
      QImage(cvLinesImage.data, cvLinesImage.cols, cvLinesImage.rows,
             static_cast<int>(cvLinesImage.step), QImage::Format_RGB888);
  qImg.save(filename.c_str());
}

static void
saveContoursImageForDebugging(const std::string &filename, const QSize &imgSize,
                              const std::vector<ContourLine> &contourLines,
                              const std::vector<Boundary> &boundaries = {}) {
  std::vector<std::vector<cv::Point>> lines;
  for (const auto &cl : contourLines) {
    lines.push_back(cl.points);
  }
  saveContoursImageForDebugging(filename, imgSize, lines, boundaries);
}

Contours::Contours(
    const QImage &img, const std::vector<QRgb> &compartmentColours,
    const std::vector<std::pair<std::string, ColourPair>> &membraneColourPairs)
    : boundaryPixelsImage(img.size(), QImage::Format_ARGB32_Premultiplied) {

  boundaryPixelsImage.fill(qRgba(0, 0, 0, 0));
  auto contours = getContours(img, compartmentColours);
#if SPDLOG_ACTIVE_LEVEL <= SPDLOG_LEVEL_TRACE
  saveContoursImageForDebugging("contours_original.png", img.size(), contours);
#endif

  boundaries = extractClosedLoopBoundaries(img, membraneColourPairs, contours,
                                           boundaryPixelsImage);
  SPDLOG_DEBUG("Number of closed loops: {}", boundaries.size());

  auto lines = extractLinesFromContours(img, contours);
  SPDLOG_DEBUG("Number of lines: {}", lines.size());
#if SPDLOG_ACTIVE_LEVEL <= SPDLOG_LEVEL_TRACE
  saveContoursImageForDebugging("contours_split_lines.png", img.size(), lines,
                                boundaries);
#endif

  removeAdjacentMembraneLines(lines, img, membraneColourPairs);
#if SPDLOG_ACTIVE_LEVEL <= SPDLOG_LEVEL_TRACE
  saveContoursImageForDebugging("contours_remove_adjacent.png", img.size(),
                                lines, boundaries);
#endif
  cv::Mat cvImg(img.height(), img.width(), CV_32SC1, cv::Scalar(0));
  mergeEndPointTriplets(lines, cvImg);
#if SPDLOG_ACTIVE_LEVEL <= SPDLOG_LEVEL_TRACE
  saveContoursImageForDebugging("contours_merge_endpoints.png", img.size(),
                                lines, boundaries);
#endif

  // add lines to contours image
  for (size_t i = 0; i < lines.size(); i++) {
    auto c = utils::indexedColours()[i + boundaries.size()].rgb();
    for (const auto &p : lines[i].points) {
      boundaryPixelsImage.setPixel(p.x, p.y, c);
    }
  }

  // index fixed points, create membrane boundaries with fixed point info
  for (const auto &line : lines) {
    auto points = toQPointsInvertYAxis(line.points, img.height());
    std::size_t startFPIndex = getOrInsertFPIndex(points.front(), fixedPoints);
    std::size_t endFPIndex = getOrInsertFPIndex(points.back(), fixedPoints);
    auto &startLine = fixedPoints[startFPIndex].lines.emplace_back();
    startLine.boundaryIndex = boundaries.size();
    startLine.startsFromFP = true;
    startLine.isMembrane = !line.membraneName.empty();
    auto &endLine = fixedPoints[endFPIndex].lines.emplace_back();
    endLine.boundaryIndex = boundaries.size();
    endLine.startsFromFP = false;
    endLine.isMembrane = !line.membraneName.empty();
    auto boundary = Boundary(points, false, !line.membraneName.empty(),
                             line.membraneName, line.membraneIndex);
    boundary.setFpIndices({startFPIndex, endFPIndex});
    if (boundary.isValid()) {
      boundaries.push_back(std::move(boundary));
    } else {
      SPDLOG_WARN("  -> ignoring invalid boundary line");
    }
  }
}

const QImage &Contours::getBoundaryPixelsImage() const {
  return boundaryPixelsImage;
}

std::vector<FixedPoint> &Contours::getFixedPoints() { return fixedPoints; }

std::vector<Boundary> &Contours::getBoundaries() { return boundaries; }

} // namespace mesh
