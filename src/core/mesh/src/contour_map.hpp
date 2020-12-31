// ContourMap

#pragma once

#include "boundary.hpp"
#include <QSize>
#include <array>
#include <opencv2/core/types.hpp>
#include <vector>

namespace mesh {

using ContourIndices = std::array<int, 4>;

class ContourMap {
private:
  std::vector<ContourIndices> indices;
  int L;

public:
  ContourMap(const QSize &size, const Contours &contours);
  const ContourIndices& getContourIndices(const cv::Point &p) const;
  bool isFixedPoint(const cv::Point &p) const;
};

} // namespace mesh
