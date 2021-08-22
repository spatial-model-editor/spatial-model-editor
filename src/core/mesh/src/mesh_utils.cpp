#include "mesh_utils.hpp"
#include <algorithm>

namespace sme::mesh {

cv::Mat makeBinaryMask(const QImage &img, const std::vector<QRgb> &cols) {
  cv::Mat m(img.height(), img.width(), CV_8UC1, cv::Scalar(0));
  for (int y = 0; y < img.height(); ++y) {
    for (int x = 0; x < img.width(); ++x) {
      if (std::find(cols.cbegin(), cols.cend(), img.pixel(x, y)) !=
          cols.cend()) {
        m.at<uint8_t>(y, x) = 255;
      }
    }
  }
  return m;
}

cv::Mat makeBinaryMask(const QImage &img, QRgb col) {
  return makeBinaryMask(img, std::vector<QRgb>{col});
}

} // namespace sme::mesh
