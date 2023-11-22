#include "sme/image_stack.hpp"
#include "sme/logger.hpp"
#include "sme/tiff.hpp"
#include "sme/utils.hpp"
#include <stdexcept>

namespace sme::common {

ImageStack::ImageStack() = default;

ImageStack::ImageStack(const Volume &size, QImage::Format format)
    : imgs{size.depth(), QImage(size.width(), size.height(), format)},
      sz{size} {}

ImageStack::ImageStack(std::vector<QImage> &&images)
    : imgs{std::move(images)},
      sz{imgs[0].width(), imgs[0].height(), imgs.size()} {}
ImageStack::ImageStack(const std::vector<QImage> &images)
    : imgs{images}, sz{imgs[0].width(), imgs[0].height(), imgs.size()} {}

ImageStack::ImageStack(const QString &filename) {
  SPDLOG_DEBUG("Import image file '{}'", filename.toStdString());
  SPDLOG_DEBUG("  - trying tiffReader");
  sme::common::TiffReader tiffReader(filename.toStdString());
  if (!tiffReader.empty()) {
    *this = tiffReader.getImages();
    return;
  }
  SPDLOG_DEBUG("    -> tiffReader could not read file");
  SPDLOG_DEBUG("  - trying QImage::load()");
  if (QImage img; img.load(filename)) {
    *this = ImageStack({img});
    return;
  }
  SPDLOG_DEBUG("    -> QImage::load() could not read file - giving up");
  throw std::invalid_argument(tiffReader.getErrorMessage().toStdString());
}

ImageStack::ImageStack(const Volume &imageSize,
                       const std::vector<double> &values, double maxValue)
    : imgs{imageSize.depth(),
           QImage(imageSize.width(), imageSize.height(), QImage::Format_RGB32)},
      sz{imageSize} {
  if (values.size() != imageSize.nVoxels()) {
    fill(0);
    return;
  }
  double scaleFactor{0.0};
  if (maxValue < 0) {
    // use maximum value from supplied values
    maxValue = common::max(values);
  }
  if (maxValue > 0.0) {
    scaleFactor = 255.0 / maxValue;
  }
  auto value{values.cbegin()};
  for (std::size_t z = 0; z < imageSize.depth(); ++z) {
    for (int y = imageSize.height() - 1; y >= 0; --y) {
      for (int x = 0; x < imageSize.width(); ++x) {
        auto intensity{static_cast<int>(scaleFactor * (*value))};
        intensity = std::clamp(intensity, 0, 255);
        imgs[z].setPixel(x, y, qRgb(intensity, intensity, intensity));
        ++value;
      }
    }
  }
}

void ImageStack::clear() {
  imgs.clear();
  sz = {0, 0, 0};
}

void ImageStack::fill(uint pixel) {
  for (auto &img : imgs) {
    img.fill(pixel);
  }
}

void ImageStack::rescaleXY(QSize size) {
  for (auto &img : imgs) {
    img = img.scaled(size, Qt::IgnoreAspectRatio, Qt::SmoothTransformation);
  }
  sz = {size.width(), size.height(), sz.depth()};
}

void ImageStack::flipYAxis() {
  for (auto &img : imgs) {
    img = img.mirrored();
  }
}

static QList<QRgb> getCombinedColorTable(const std::vector<QImage> &images) {
  // construct a colorTable that contains all colors from all slices
  // todo: put this in image_stack_impl, add tests and benchmarks, optimize?
  constexpr auto flagNoDither{Qt::AvoidDither | Qt::ThresholdDither |
                              Qt::ThresholdAlphaDither | Qt::NoOpaqueDetection};
  QList<QRgb> colorTable{};
  for (auto &image : images) {
    auto sliceColorTable{
        image.convertToFormat(QImage::Format_Indexed8, flagNoDither)
            .colorTable()};
    for (auto color : sliceColorTable) {
      if (!colorTable.contains(color)) {
        colorTable.push_back(color);
      }
    }
  }
  return colorTable;
}

void ImageStack::convertToIndexed() {
  constexpr auto flagNoDither{Qt::AvoidDither | Qt::ThresholdDither |
                              Qt::ThresholdAlphaDither | Qt::NoOpaqueDetection};
  for (auto &img : imgs) {
    if (img.hasAlphaChannel()) {
      img = img.convertToFormat(QImage::Format_RGB32, flagNoDither);
    }
  }
  auto colorTable{getCombinedColorTable(imgs)};
  for (auto &img : imgs) {
    img =
        img.convertToFormat(QImage::Format_Indexed8, colorTable, flagNoDither);
  }
}

ImageStack ImageStack::scaled(int width, int height) {
  ImageStack scaled{*this};
  for (std::size_t z = 0; z < sz.depth(); ++z) {
    scaled.imgs[z] = imgs[z].scaled(width, height, Qt::IgnoreAspectRatio,
                                    Qt::FastTransformation);
    scaled.sz = {scaled.imgs[z].size(), sz.depth()};
  }
  return scaled;
}

ImageStack ImageStack::scaledToWidth(int width) {
  ImageStack scaled{*this};
  for (std::size_t z = 0; z < sz.depth(); ++z) {
    scaled.imgs[z] = imgs[z].scaledToWidth(width, Qt::FastTransformation);
    scaled.sz = {scaled.imgs[z].size(), sz.depth()};
  }
  return scaled;
}

ImageStack ImageStack::scaledToHeight(int height) {
  ImageStack scaled{*this};
  for (std::size_t z = 0; z < sz.depth(); ++z) {
    scaled.imgs[z] = imgs[z].scaledToHeight(height, Qt::FastTransformation);
    scaled.sz = {scaled.imgs[z].size(), sz.depth()};
  }
  return scaled;
}
} // namespace sme::common
