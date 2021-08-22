#pragma once

#include "line_simplifier.hpp"
#include <QImage>
#include <QPoint>
#include <QPointF>
#include <cstddef>
#include <limits>
#include <opencv2/core/types.hpp>
#include <string>
#include <vector>

namespace sme::mesh {

/**
 * @brief Approximate boundary line with adjustable number of points
 *
 * Given an ordered set of pixels that form a line or a closed loop, constructs
 * an approximation to the line. The number of points used can be chosen
 * automatically or set by the user.
 */
class Boundary {
private:
  LineSimplifier lineSimplifier;
  std::size_t maxPoints{};
  std::vector<QPoint> points;

public:
  /**
   * @brief Is the line a closed loop
   */
  [[nodiscard]] bool isLoop() const;
  /**
   * @brief Is the line valid
   */
  [[nodiscard]] bool isValid() const;
  /**
   * @brief The simplified line
   */
  [[nodiscard]] const std::vector<QPoint> &getPoints() const;
  /**
   * @brief The original un-simplified line
   */
  [[nodiscard]] const std::vector<QPoint> &getAllPoints() const;
  /**
   * @brief The maximum number of points used by the approximate line
   */
  [[nodiscard]] std::size_t getMaxPoints() const;
  /**
   * @brief Set the maximum number of points to use in the approximate line
   */
  void setMaxPoints(std::size_t maxPoints);
  /**
   * @brief Automatically choose and return the maximum number of points to use
   * in the approximate line
   */
  std::size_t setMaxPoints();
  /**
   * @brief Construct a simplified line from on ordered set of points
   *
   * @param[in] boundaryPoints the boundary as an ordered set of points
   * @param[in] isClosedLoop true if the last point implicitly connects to the
   * first point to form a closed loop
   */
  explicit Boundary(const std::vector<QPoint> &boundaryPoints,
                    bool isClosedLoop = false);
};

/**
 * @brief Construct all the compartment boundaries from an image
 *
 * @param[in] compartmentImage the segmented compartment geometry image
 * @param[in] compartmentColours the colours that correspond to compartments
 */
std::vector<Boundary>
constructBoundaries(const QImage &compartmentImage,
                    const std::vector<QRgb> &compartmentColours);

} // namespace sme::mesh
