#include "contour_map.hpp"
#include <QColor>
#include <QPoint>
#include <QSize>
#include <algorithm>
#include <limits>
#include <opencv2/core/cvdef.h>
#include <opencv2/core/hal/interface.h>
#include <opencv2/core/matx.hpp>
#include <opencv2/imgproc.hpp>

namespace mesh {

ContourMap::ContourMap(const QSize &size,
                       const std::vector<std::vector<cv::Point>> &contours)
    : indices(size.height() + 2, size.width() + 2, CV_32SC1, cv::Scalar(-1)) {
  int contourIndex{0};
  for (const auto &contour : contours) {
    for (const auto &pixel : contour) {
      indices.at<int>(pixel.y + 1, pixel.x + 1) = contourIndex;
    }
    ++contourIndex;
  }
}

std::size_t ContourMap::getContourIndex(const cv::Point &p) const {
  auto i = indices.at<int>(p.y + 1, p.x + 1);
  if (i == -1) {
    return nullIndex;
  }
  return static_cast<std::size_t>(i);
}

// looks for an adjacent pixel that belongs to another contour
// if found returns contour index, otherwise returns nullIndex
std::size_t ContourMap::getAdjacentContourIndex(const cv::Point &p) const {
  auto pci = getContourIndex(p);
  for (const auto &dp : nn4) {
    auto pp = p + dp;
    if (auto nci = getContourIndex(pp); nci != pci && nci != nullIndex) {
      return nci;
    }
  }
  return nullIndex;
}

// if given a valid contourIndex:
//  - returns true if there is an adjacent pixel to p from this contour
// if given nullIndex as contourIndex:
//  - returns true if *all* adjacent pixels are not part of another contour
bool ContourMap::hasNeighbourWithThisIndex(const cv::Point &p,
                                           std::size_t contourIndex) const {
  if (contourIndex == nullIndex) {
    auto pci = getContourIndex(p);
    return std::all_of(nn4.cbegin(), nn4.cend(),
                       [&p, pci, this](const auto &dp) {
                         auto pp = p + dp;
                         auto i = getContourIndex(pp);
                         return i == pci || i == nullIndex;
                       });
  }
  return std::any_of(nn4.cbegin(), nn4.cend(),
                     [&p, contourIndex, this](const auto &dp) {
                       auto pp = p + dp;
                       auto i = static_cast<std::size_t>(getContourIndex(pp));
                       return i == contourIndex;
                     });
}

bool ContourMap::hasNeighbourWithThisIndex(const std::vector<cv::Point> &points,
                                           std::size_t contourIndex) const {
  return std::all_of(points.cbegin(), points.cend(),
                     [this, contourIndex](const cv::Point &p) {
                       return hasNeighbourWithThisIndex(p, contourIndex);
                     });
}

} // namespace mesh
