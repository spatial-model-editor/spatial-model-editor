#pragma once

#include "boundary.hpp"
#include <QImage>
#include <QPoint>
#include <QPointF>
#include <string>
#include <vector>

namespace sme::mesh {

/**
 * @brief Approximate boundary lines for a segmented image
 *
 * Constructs a set of boundary lines or loops that approximately
 * separate the original segmented image into regions of the same colour.
 * The boundary lines can then be simplified, either independently or
 * all together in a topology preserving way.
 */
class Boundaries {
private:
  std::vector<Boundary> boundaries{};

public:
  /**
   * @brief Construct all the compartment boundaries from an image
   *
   * @param[in] compartmentImage the segmented compartment geometry image
   * @param[in] compartmentColours the colours that correspond to compartments
   */
  explicit Boundaries(const QImage &compartmentImage,
                      const std::vector<QRgb> &compartmentColours);
  Boundaries() = default;

  [[nodiscard]] const std::vector<Boundary> &getBoundaries() const;
  /**
   * @brief The number of boundaries
   */
  [[nodiscard]] inline std::size_t size() const { return boundaries.size(); }
  /**
   * @brief Set the maximum number of allowed points for a given boundary
   *
   * @param[in] boundaryIndex the index of the boundary
   * @param[in] maxPoints the maximum number of points allowed
   */
  void setMaxPoints(std::size_t boundaryIndex, std::size_t maxPoints);
  /**
   * @brief Get the maximum number of allowed points for a given boundary
   *
   * @param[in] boundaryIndex the index of the boundary
   */
  [[nodiscard]] std::size_t getMaxPoints(std::size_t boundaryIndex) const;
  /**
   * @brief The maximum allowed points for each boundary in the mesh
   */
  [[nodiscard]] std::vector<std::size_t> getMaxPoints() const;
  /**
   * @brief Automatically choose the maximum number of points for each boundary
   *
   */
  void setMaxPoints();
};

} // namespace sme::mesh
