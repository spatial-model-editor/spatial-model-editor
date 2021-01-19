#include "mesh_utils.hpp"
#include <algorithm>

namespace sme {

namespace mesh {

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

std::optional<cv::Point> getNonZeroPixel(const cv::Mat &img) {
  const int M{img.rows};
  const int N{img.cols};
  for (int m = 0; m < M; ++m) {
    const uchar *row{img.ptr(m)};
    for (int n = 0; n < N; ++n) {
      if (row[n] > 0) {
        return cv::Point(n, m);
      }
    }
  }
  return {};
}

} // namespace mesh

} // namespace sme
