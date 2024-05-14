// Python.h (#included by nanobind.h) must come first
// https://docs.python.org/3.2/c-api/intro.html#include-files
#include <nanobind/nanobind.h>

#include "sme_common.hpp"

namespace pysme {

// 4d array with triplet of r,g,b uint8 values for each pixel
nanobind::ndarray<nanobind::numpy, std::uint8_t>
toPyImageRgb(const ::sme::common::ImageStack &imgs) {
  auto d = imgs.volume().depth();
  auto h = imgs.volume().height();
  auto w = imgs.volume().width();
  std::vector<std::uint8_t> a;
  a.reserve(d * h * w * 3);
  for (std::size_t z = 0; z < d; ++z) {
    for (int y = 0; y < h; ++y) {
      for (int x = 0; x < w; ++x) {
        auto c{imgs[z].pixel(x, y)};
        a.push_back(static_cast<std::uint8_t>(qRed(c)));
        a.push_back(static_cast<std::uint8_t>(qGreen(c)));
        a.push_back(static_cast<std::uint8_t>(qBlue(c)));
      }
    }
  }
  return as_ndarray(std::move(a), {d, static_cast<std::size_t>(h),
                                   static_cast<std::size_t>(w), 3});
}

// 3d array of bool for each pixel
nanobind::ndarray<nanobind::numpy, bool>
toPyImageMask(const ::sme::common::ImageStack &imgs) {
  auto d = imgs.volume().depth();
  auto h = imgs.volume().height();
  auto w = imgs.volume().width();
  std::vector<std::uint8_t> a;
  a.reserve(d * h * w * 3);
  for (std::size_t z = 0; z < d; ++z) {
    for (int y = 0; y < h; ++y) {
      for (int x = 0; x < w; ++x) {
        // a numpy array of bool is stored as an array of bytes:
        // https://numpy.org/doc/stable/reference/arrays.scalars.html#numpy.bool_
        // not documented but they use 0 for False and 1 for True:
        // https://github.com/numpy/numpy/blob/3d33d5f29baa5477e1f725310f0a1f94fb723108/numpy/_core/include/numpy/npy_common.h#L303-L305
        a.push_back(static_cast<std::uint8_t>(imgs[z].pixelIndex(x, y) > 0));
      }
    }
  }
  return as_ndarray<std::uint8_t, bool>(
      std::move(a),
      {d, static_cast<std::size_t>(h), static_cast<std::size_t>(w)});
}

} // namespace pysme
