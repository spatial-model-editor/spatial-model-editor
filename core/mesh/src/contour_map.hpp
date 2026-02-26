// ContourMap

#pragma once

#include <QSize>
#include <array>
#include <opencv2/core/types.hpp>
#include <vector>

namespace sme::mesh {

/**
 * @brief Up to four contour ids incident on a grid corner.
 */
using ContourIndices = std::array<int, 4>;

/**
 * @brief Pixel contours extracted from segmented geometry.
 */
struct Contours {
  /**
   * @brief Compartment boundary contours.
   */
  std::vector<std::vector<cv::Point>> compartmentEdges;
  /**
   * @brief Outer domain boundary contours.
   */
  std::vector<std::vector<cv::Point>> domainEdges;
};

/**
 * @brief Mapping from corner points to nearby contour ids.
 */
class ContourMap {
private:
  std::vector<ContourIndices> indices;
  int L;

public:
  /**
   * @brief Construct map for a given image size and contour set.
   */
  ContourMap(const QSize &size, const Contours &contours);
  /**
   * @brief Contour ids incident on corner ``p``.
   */
  [[nodiscard]] const ContourIndices &
  getContourIndices(const cv::Point &p) const;
  /**
   * @brief Returns ``true`` if corner ``p`` belongs to at least three
   * contours.
   */
  [[nodiscard]] bool isFixedPoint(const cv::Point &p) const;
};

} // namespace sme::mesh
