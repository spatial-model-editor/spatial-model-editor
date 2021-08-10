// ContourMap

#pragma once

#include "boundary.hpp"
#include <QSize>
#include <array>
#include <opencv2/core/types.hpp>
#include <vector>

namespace sme {

namespace mesh {

using ContourIndices = std::array<int, 4>;

class ContourMap {
private:
  std::vector<ContourIndices> indices;
  int L;

public:
  ContourMap(const QSize &size, const Contours &contours);
  [[nodiscard]] const ContourIndices &
  getContourIndices(const cv::Point &p) const;
  [[nodiscard]] bool isFixedPoint(const cv::Point &p) const;
};

} // namespace mesh

} // namespace sme
