#include "sme_common.hpp"

namespace sme {

// 3d array with triplet of r,g,b uint8 values for each pixel
pybind11::array toPyImageRgb(const QImage &img) {
  pybind11::array_t<std::uint8_t> a({img.height(), img.width(), 3});
  auto r{a.mutable_unchecked<3>()};
  for (int y = 0; y < img.height(); ++y) {
    for (int x = 0; x < img.width(); ++x) {
      auto c{img.pixel(x, y)};
      r(y, x, 0) = static_cast<std::uint8_t>(qRed(c));
      r(y, x, 1) = static_cast<std::uint8_t>(qGreen(c));
      r(y, x, 2) = static_cast<std::uint8_t>(qBlue(c));
    }
  }
  return a;
}

// 2d array of bool for each pixel
pybind11::array toPyImageMask(const QImage &img) {
  pybind11::array_t<bool> a({img.height(), img.width()});
  auto r{a.mutable_unchecked<2>()};
  for (int y = 0; y < img.height(); ++y) {
    for (int x = 0; x < img.width(); ++x) {
      r(y, x) = img.pixelIndex(x, y) > 0;
    }
  }
  return a;
}

} // namespace sme
