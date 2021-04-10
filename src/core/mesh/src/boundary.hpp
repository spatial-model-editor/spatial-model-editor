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
 * @brief A planar line segment boundary line
 *
 * A planar line segment approximation to a boundary, where the maximum number
 * of points used for the approximation can be altered.
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

/** @brief Constructs a set of planar line segment approximate boundary lines
 *
 * Given a segmented geometry image and the colours of the compartments in
 * the image, constructs a set of boundaries that approximate all compartment
 * boundaries.
 *
 * @param[in] img segmented geometry image
 * @param[in] compartmentColours the colours of all the compartments in the
 * model
 * @param[out] pixelCorners an image of the pixel corners, colour coded as
 * either black (not part of a boundary), blue (part of a straight boundary
 * segment), or red (a corner or inflexion point of a boundary)
 *
 */
std::vector<Boundary>
constructBoundaries(const QImage &img,
                    const std::vector<QRgb> &compartmentColours,
                    QImage &pixelCorners);

} // namespace sme::mesh
