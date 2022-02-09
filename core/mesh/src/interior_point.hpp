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

std::vector<std::vector<QPointF>>
getInteriorPoints(const QImage &img, const std::vector<QRgb> &cols);

} // namespace sme::mesh
