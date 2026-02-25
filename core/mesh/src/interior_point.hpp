// InteriorPoint
//  - takes an image and a vector of colors
//  - for each color
//    - identifies each connected region of that color
//    - find an interior point for each connected region

#pragma once
#include <QImage>
#include <QPointF>
#include <opencv2/core/types.hpp>
#include <vector>

namespace sme::mesh {

/**
 * @brief Find representative interior points for each connected region.
 *
 * For each color in ``cols``, connected components are identified in ``img``
 * and one interior point is returned per component.
 */
std::vector<std::vector<QPointF>>
getInteriorPoints(const QImage &img, const std::vector<QRgb> &cols);

} // namespace sme::mesh
