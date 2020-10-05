#include "sme_common.hpp"

namespace sme {

PyImageRgb toPyImageRgb(const QImage &img) {
  std::size_t h = static_cast<std::size_t>(img.height());
  std::size_t w = static_cast<std::size_t>(img.width());
  PyImageRgb v(h, std::vector<std::vector<int>>(w, std::vector<int>(3, 0)));
  for (std::size_t y = 0; y < h; ++y) {
    for (std::size_t x = 0; x < w; ++x) {
      auto c = img.pixel(static_cast<int>(x), static_cast<int>(y));
      v[y][x][0] = qRed(c);
      v[y][x][1] = qGreen(c);
      v[y][x][2] = qBlue(c);
    }
  }
  return v;
}

PyImageMask toPyImageMask(const QImage &img) {
  std::size_t h = static_cast<std::size_t>(img.height());
  std::size_t w = static_cast<std::size_t>(img.width());
  PyImageMask v(h, std::vector<bool>(w, false));
  for (std::size_t y = 0; y < h; ++y) {
    for (std::size_t x = 0; x < w; ++x) {
      v[y][x] = img.pixelIndex(static_cast<int>(x), static_cast<int>(y)) > 0;
    }
  }
  return v;
}

} // namespace sme
