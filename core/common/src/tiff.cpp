#include "sme/tiff.hpp"
#include "sme/logger.hpp"
#include "sme/utils.hpp"
#include <QPoint>
#include <QSize>
#include <algorithm>
#include <limits>
#include <memory>
#include <tiff.h>
#include <tiffio.h>

namespace sme::common {

constexpr int TiffDataTypeBits = std::numeric_limits<TiffDataType>::digits;
constexpr TiffDataType TiffDataTypeMaxValue =
    std::numeric_limits<TiffDataType>::max();

double writeTIFF(const std::string &filename, const QSize &imageSize,
                 const std::vector<double> &conc, double pixelWidth) {
  // todo: add ORIGIN to TIFF file!
  SPDLOG_TRACE("found {} concentration values", conc.size());
  double maxConc = *std::max_element(conc.cbegin(), conc.cend());
  SPDLOG_TRACE("  - max value: {}", maxConc);

  // convert to array of type TiffDataType
  SPDLOG_TRACE("using TIFF data type: {}", decltypeStr<TiffDataType>());
  SPDLOG_TRACE("  - size in bits: {}", TiffDataTypeBits);
  SPDLOG_TRACE("  - max value: {}", TiffDataTypeMaxValue);
  const auto width{static_cast<std::size_t>(imageSize.width())};
  const auto height{static_cast<std::size_t>(imageSize.height())};
  SPDLOG_TRACE("  - image size {}x{}", width, height);
  auto tifValues = std::vector<std::vector<TiffDataType>>(
      height, std::vector<TiffDataType>(width, 0));
  SPDLOG_TRACE("{}", TiffDataTypeMaxValue);
  const double scaleFactor{TiffDataTypeMaxValue / maxConc};
  SPDLOG_TRACE("{}", scaleFactor);
  for (std::size_t y = 0; y < height; ++y) {
    for (std::size_t x = 0; x < width; ++x) {
      auto val = static_cast<TiffDataType>(scaleFactor *
                                           conc[x + width * (height - 1 - y)]);
      tifValues[y][x] = val;
    }
  }

  // write to TIFF
  TIFF *tif = TIFFOpen(filename.c_str(), "w");
  if (tif == nullptr) {
    SPDLOG_ERROR("Failed to open file {} for writing", filename);
    return -1;
  }
  TIFFSetField(tif, TIFFTAG_IMAGEWIDTH, width);
  TIFFSetField(tif, TIFFTAG_IMAGELENGTH, height);
  // one unsigned int per pixel:
  TIFFSetField(tif, TIFFTAG_SAMPLEFORMAT, SAMPLEFORMAT_UINT);
  TIFFSetField(tif, TIFFTAG_BITSPERSAMPLE, TiffDataTypeBits);
  TIFFSetField(tif, TIFFTAG_SAMPLESPERPIXEL, 1);
  // only affects how the data is stored
  TIFFSetField(tif, TIFFTAG_ROWSPERSTRIP, 8);
  // NB: same orientation as QImage: (0,0) in top left
  TIFFSetField(tif, TIFFTAG_ORIENTATION, ORIENTATION_TOPLEFT);
  TIFFSetField(tif, TIFFTAG_PLANARCONFIG, PLANARCONFIG_CONTIG);
  // NB: black == 0
  TIFFSetField(tif, TIFFTAG_PHOTOMETRIC, PHOTOMETRIC_MINISBLACK);
  TIFFSetField(tif, TIFFTAG_COMPRESSION, COMPRESSION_NONE);
  // NB: ignored by DUNE:
  TIFFSetField(tif, TIFFTAG_RESOLUTIONUNIT, RESUNIT_CENTIMETER);
  // NB: factor to convert from pixel location to physical location
  // pixelX = TIFFTAG_XRESOLUTION * physicalX, etc.
  TIFFSetField(tif, TIFFTAG_XRESOLUTION, 1.0 / pixelWidth);
  TIFFSetField(tif, TIFFTAG_YRESOLUTION, 1.0 / pixelWidth);
  // NB: location of (0,0) pixel
  TIFFSetField(tif, TIFFTAG_XPOSITION, 0.0);
  TIFFSetField(tif, TIFFTAG_YPOSITION, 0.0);

  for (std::size_t y = 0; y < height; y++) {
    int ret = TIFFWriteScanline(tif, tifValues.at(y).data(),
                                static_cast<uint32_t>(y));
    if (ret != 1) {
      SPDLOG_ERROR("TIFFWriteScanline error on row {}", y);
    }
  }
  TIFFClose(tif);

  return maxConc;
}

template <typename T>
std::vector<double> readLineToDoubles(TIFF *tif, std::size_t y,
                                      std::size_t width) {
  std::vector<double> dblValues;
  dblValues.reserve(width);
  std::vector<T> tiffValues(width, 0);
  TIFFReadScanline(tif, tiffValues.data(), static_cast<uint32_t>(y));
  for (auto tiffValue : tiffValues) {
    dblValues.push_back(static_cast<double>(tiffValue));
  }
  return dblValues;
}

TiffReader::TiffReader(const std::string &filename) {
  TIFF *tif = TIFFOpen(filename.c_str(), "r");
  if (tif == nullptr) {
    SPDLOG_WARN("Failed to open file {}", filename);
    return;
  }
  SPDLOG_INFO("File {} contains", filename);
  do {
    std::size_t width = 0;
    std::size_t height = 0;
    std::size_t samplespp = 0;
    std::size_t bitspp = 0;
    std::size_t samplefmt = 0;

    bool ok = true;
    if (TIFFGetField(tif, TIFFTAG_IMAGEWIDTH, &width) != 1) {
      errorMessage = "failed to read TIFFTAG_IMAGEWIDTH";
      SPDLOG_DEBUG("  - {}", errorMessage.toStdString());
      ok = false;
    }
    if (TIFFGetField(tif, TIFFTAG_IMAGELENGTH, &height) != 1) {
      errorMessage = "failed to read TIFFTAG_IMAGELENGTH";
      SPDLOG_DEBUG("  - {}", errorMessage.toStdString());
      ok = false;
    }
    if (TIFFGetField(tif, TIFFTAG_SAMPLESPERPIXEL, &samplespp) != 1) {
      SPDLOG_DEBUG("  - failed to read TIFFTAG_SAMPLESPERPIXEL: assuming 1");
      samplespp = 1;
    }
    if (TIFFGetField(tif, TIFFTAG_BITSPERSAMPLE, &bitspp) != 1) {
      errorMessage = "failed to read TIFFTAG_BITSPERSAMPLE";
      SPDLOG_DEBUG("  - {}", errorMessage.toStdString());
      ok = false;
    }
    if (TIFFGetField(tif, TIFFTAG_SAMPLEFORMAT, &samplefmt) != 1) {
      SPDLOG_DEBUG("  - failed to read TIFFTAG_SAMPLEFORMAT: assuming "
                   "SAMPLEFORMAT_UINT");
      samplefmt = SAMPLEFORMAT_UINT;
    }
    if (ok) {
      SPDLOG_DEBUG("  - {}x{} image", width, height);
      SPDLOG_DEBUG("    - {} samples per pixel", samplespp);
      SPDLOG_DEBUG("    - {} bits per sample", bitspp);

      if (samplespp == 1) {
        SPDLOG_INFO("    --> importing grayscale image...");
        auto &img = tiffImages.emplace_back();
        img.width = width;
        img.height = height;
        for (std::size_t y = 0; y < height; y++) {
          if (samplefmt == SAMPLEFORMAT_UINT && bitspp == 8) {
            img.values.push_back(readLineToDoubles<uint8_t>(tif, y, width));
          } else if (samplefmt == SAMPLEFORMAT_UINT && bitspp == 16) {
            img.values.push_back(readLineToDoubles<uint16_t>(tif, y, width));
          } else if (samplefmt == SAMPLEFORMAT_UINT && bitspp == 32) {
            img.values.push_back(readLineToDoubles<uint32_t>(tif, y, width));
          } else if (samplefmt == SAMPLEFORMAT_INT && bitspp == 8) {
            img.values.push_back(readLineToDoubles<int8_t>(tif, y, width));
          } else if (samplefmt == SAMPLEFORMAT_INT && bitspp == 16) {
            img.values.push_back(readLineToDoubles<int16_t>(tif, y, width));
          } else if (samplefmt == SAMPLEFORMAT_INT && bitspp == 32) {
            img.values.push_back(readLineToDoubles<int32_t>(tif, y, width));
          } else if (samplefmt == SAMPLEFORMAT_IEEEFP && bitspp == 32) {
            img.values.push_back(readLineToDoubles<float>(tif, y, width));
          } else if (samplefmt == SAMPLEFORMAT_IEEEFP && bitspp == 64) {
            img.values.push_back(readLineToDoubles<double>(tif, y, width));
          } else {
            errorMessage = QString("%1-bit SAMPLEFORMAT enum %2 not supported")
                               .arg(bitspp)
                               .arg(samplefmt);
            break;
          }
          auto [minV, maxV] = common::minmax(img.values.back());
          img.maxValue = std::max(maxV, img.maxValue);
          img.minValue = std::min(minV, img.minValue);
        }
        SPDLOG_DEBUG("    - min value: {}", img.minValue);
        SPDLOG_DEBUG("    - max value: {}", img.maxValue);
        maxValue = std::max(maxValue, img.maxValue);
        minValue = std::min(minValue, img.minValue);
      }
    }
  } while (TIFFReadDirectory(tif) != 0);
  TIFFClose(tif);
}

std::size_t TiffReader::size() const { return tiffImages.size(); }

sme::common::ImageStack TiffReader::getImages() const {
  std::vector<QImage> imageVector{};
  imageVector.reserve(tiffImages.size());
  for (const auto &tiffImage : tiffImages) {
    auto &image = imageVector.emplace_back(static_cast<int>(tiffImage.width),
                                           static_cast<int>(tiffImage.height),
                                           QImage::Format_RGB32);
    double maxVal{maxValue};
    // check for case of all zero's: should be black image
    if (maxVal == 0) {
      maxVal = 1.0;
    }
    for (int y = 0; y < image.height(); ++y) {
      const auto &row = tiffImage.values[static_cast<std::size_t>(y)];
      for (int x = 0; x < image.width(); ++x) {
        // rescale pixel values from [0, max] to [0,255]
        double unitNormValue = row[static_cast<std::size_t>(x)] / maxVal;
        unsigned int mask8bit{0x0000ff};
        unsigned int val8 =
            mask8bit & static_cast<unsigned int>(255 * unitNormValue);
        unsigned int rgb = 0xff000000 | val8 | (val8 << 8) | (val8 << 16);
        image.setPixel(x, y, rgb);
      }
    }
  }
  return sme::common::ImageStack(std::move(imageVector));
}

const QString &TiffReader::getErrorMessage() const { return errorMessage; }

} // namespace sme::common
