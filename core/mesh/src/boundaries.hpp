#pragma once

#include "boundary.hpp"
#include "polyline_simplifier.hpp"
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
 * separate the original segmented image into regions of the same color.
 * The boundary lines can then be simplified, either independently or
 * all together in a topology preserving way.
 */
class Boundaries {
private:
  std::vector<Boundary> m_boundaries{};
  std::size_t m_simplifierType{0};
  PolylineSimplifier m_polylineSimplifier;

public:
  /**
   * @brief Construct all the compartment boundaries from an image
   *
   * @param[in] compartmentImage the segmented compartment geometry image
   * @param[in] compartmentColors the colors that correspond to compartments
   */
  explicit Boundaries(const QImage &compartmentImage,
                      const std::vector<QRgb> &compartmentColors,
                      std::size_t simplifierType);
  Boundaries() = default;
  ~Boundaries();
  /**
   * @brief The simplified boundaries
   */
  [[nodiscard]] const std::vector<Boundary> &getBoundaries() const;
  /**
   * @brief Get the type of boundary simplifier used
   */
  [[nodiscard]] std::size_t getSimplifierType() const;
  /**
   * @brief Set the type of boundary simplifier used
   */
  void setSimplifierType(std::size_t simplifierType);
  /**
   * @brief The number of boundaries
   */
  [[nodiscard]] inline std::size_t size() const { return m_boundaries.size(); }
  /**
   * @brief Set the maximum number of allowed points for a given boundary
   *
   * @param[in] boundaryIndex the index of the boundary
   * @param[in] maxPoints the maximum number of points allowed
   */
  void setMaxPoints(std::size_t boundaryIndex, std::size_t maxPoints);
  /**
   * @brief Set the total maximum number of allowed points over all boundaries
   *
   * @param[in] maxPoints the maximum number of points allowed
   */
  void setMaxPoints(std::size_t maxPoints);
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
