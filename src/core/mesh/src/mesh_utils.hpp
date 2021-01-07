// Mesh utils

#pragma once
#include <QImage>
#include <QRgb>
#include <opencv2/core.hpp>
#include <optional>
#include <vector>

namespace mesh {

cv::Mat makeBinaryMask(const QImage &img, const std::vector<QRgb> &cols);
cv::Mat makeBinaryMask(const QImage &img, QRgb col);
std::optional<cv::Point> getNonZeroPixel(const cv::Mat &img);

} // namespace mesh
