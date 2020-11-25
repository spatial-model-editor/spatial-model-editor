// ContourMap

#pragma once

#include "boundary.hpp"
#include "mesh_types.hpp"
#include <QImage>
#include <QRgb>
#include <array>
#include <cstddef>
#include <opencv2/core/mat.hpp>
#include <opencv2/core/mat.inl.hpp>
#include <opencv2/core/types.hpp>
#include <optional>
#include <string>
#include <utility>
#include <vector>

namespace mesh {

class ContourMap {
private:
  inline const static std::array<cv::Point, 4> nn4{
      cv::Point(1, 0), cv::Point(-1, 0), cv::Point(0, -1), cv::Point(0, 1)};
  cv::Mat indices;

public:
  ContourMap(const QSize &size,
             const std::vector<std::vector<cv::Point>> &contours);
  std::size_t getContourIndex(const cv::Point &p) const;
  std::size_t getAdjacentContourIndex(const cv::Point &p) const;
  bool hasNeighbourWithThisIndex(const cv::Point &p,
                                 std::size_t contourIndex) const;
  bool hasNeighbourWithThisIndex(const std::vector<cv::Point> &points,
                                 std::size_t contourIndex) const;
  static constexpr std::size_t nullIndex{std::numeric_limits<std::size_t>::max()};
};

} // namespace mesh
