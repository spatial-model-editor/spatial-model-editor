// Mesh utils

#pragma once

#include <QImage>
#include <QRgb>
#include <opencv2/core.hpp>
#include <vector>

namespace sme::mesh {

/**
 * @brief Create binary mask where selected colors are foreground.
 */
cv::Mat makeBinaryMask(const QImage &img, const std::vector<QRgb> &cols);
/**
 * @brief Create binary mask where one color is foreground.
 */
cv::Mat makeBinaryMask(const QImage &img, QRgb col);

} // namespace sme::mesh
