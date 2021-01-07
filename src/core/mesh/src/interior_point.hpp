// InteriorPoint
//  - takes an image and a vector of colours
//  - for each colour
//    - identifies each connected region of that colour
//    - find an interior point for each connected region

#pragma once
#include <opencv2/core/types.hpp>
#include <QImage>
#include <QPointF>
#include <vector>

namespace mesh {

std::vector<std::vector<QPointF>>
getInteriorPoints(const QImage &img,
                                    const std::vector<QRgb> &cols);

} // namespace mesh
