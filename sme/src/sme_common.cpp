// Python.h (included by pybind11.h) must come first
// https://docs.python.org/3.2/c-api/intro.html#include-files
#include <pybind11/pybind11.h>

#include "sme_common.hpp"

namespace sme {

// 4d array with triplet of r,g,b uint8 values for each pixel
pybind11::array toPyImageRgb(const sme::common::ImageStack &imgs) {
  auto h{imgs.volume().height()};
  auto w{imgs.volume().width()};
  auto d{static_cast<int>(imgs.volume().depth())};
  pybind11::array_t<std::uint8_t> a({d, h, w, 3});
  auto r{a.mutable_unchecked<4>()};
  for (int z = 0; z < d; ++z) {
    for (int y = 0; y < h; ++y) {
      for (int x = 0; x < w; ++x) {
        auto c{imgs[static_cast<std::size_t>(z)].pixel(x, y)};
        r(z, y, x, 0) = static_cast<std::uint8_t>(qRed(c));
        r(z, y, x, 1) = static_cast<std::uint8_t>(qGreen(c));
        r(z, y, x, 2) = static_cast<std::uint8_t>(qBlue(c));
      }
    }
  }
  return a;
}

// 3d array of bool for each pixel
pybind11::array toPyImageMask(const sme::common::ImageStack &imgs) {
  auto h{imgs.volume().height()};
  auto w{imgs.volume().width()};
  auto d{static_cast<int>(imgs.volume().depth())};
  pybind11::array_t<bool> a({d, h, w});
  auto r{a.mutable_unchecked<3>()};
  for (int z = 0; z < d; ++z) {
    for (int y = 0; y < h; ++y) {
      for (int x = 0; x < w; ++x) {
        r(z, y, x) = imgs[static_cast<std::size_t>(z)].pixelIndex(x, y) > 0;
      }
    }
  }
  return a;
}

} // namespace sme
