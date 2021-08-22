// ContourMap

#pragma once

#include <QSize>
#include <array>
#include <opencv2/core/types.hpp>
#include <vector>

namespace sme::mesh {

using ContourIndices = std::array<int, 4>;

struct Contours {
  std::vector<std::vector<cv::Point>> compartmentEdges;
  std::vector<std::vector<cv::Point>> domainEdges;
};

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

} // namespace sme::mesh
