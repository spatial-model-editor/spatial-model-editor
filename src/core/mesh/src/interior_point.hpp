// InteriorPoint
//  - takes an image and a vector of colours
//  - for each colour
//    - identifies each connected region of that colour
//    - find an interior point for each connected region

#pragma once
#include <QImage>
#include <QPointF>
#include <opencv2/core/types.hpp>
#include <vector>

namespace sme::mesh {

/**
 * @brief Find interior point(s) for each compartment
 *
 * Given a segmented geometry image, and the colours corresponding to
 * compartments, returns 1 or more interior points for each compartment.
 * One interior point is created for each connected region of pixels of the
 * same colour (so a compartment may have multiple interior points).
 * The interior points are chosen to be maximally far away from the exterior
 * boundary of the connected region, to minimize the chance of a poor
 * approximation to the boundary causing the interior point to lie outside of
 * the approximate boundary.
 *
 * @param[in] img segmented geometry image
 * @param[in] cols the colours of all the compartments in the model
 */
std::vector<std::vector<QPointF>>
getInteriorPoints(const QImage &img, const std::vector<QRgb> &cols);

} // namespace sme::mesh
