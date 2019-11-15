#include "tiff.hpp"

#include <tiffio.h>

#include <QImage>

#include "geometry.hpp"
#include "logger.hpp"
#include "utils.hpp"

namespace utils {

constexpr int TiffDataTypeBits = std::numeric_limits<TiffDataType>::digits;
constexpr TiffDataType TiffDataTypeMaxValue =
    std::numeric_limits<TiffDataType>::max();

double writeTIFF(const std::string& filename, const geometry::Field& field,
                 double pixelWidth) {
  // get concentration & max value
  const auto& conc = field.conc;
  SPDLOG_TRACE("found {} concentration values", conc.size());
  double maxConc = *std::max_element(conc.begin(), conc.end());
  SPDLOG_TRACE("  - max value: {}", maxConc);

  // convert to array of type TiffDataType
  SPDLOG_TRACE("using TIFF data type: {}", decltypeStr<TiffDataType>());
  SPDLOG_TRACE("  - size in bits: {}", TiffDataTypeBits);
  SPDLOG_TRACE("  - max value: {}", TiffDataTypeMaxValue);
  auto width =
      static_cast<std::size_t>(field.geometry->getCompartmentImage().width());
  auto height =
      static_cast<std::size_t>(field.geometry->getCompartmentImage().height());
  SPDLOG_TRACE("  - image size {}x{}", width, height);
  auto tifValues = std::vector<std::vector<TiffDataType>>(
      height, std::vector<TiffDataType>(width, 0));
  double scaleFactor = TiffDataTypeMaxValue / maxConc;
  for (std::size_t i = 0; i < field.geometry->ix.size(); ++i) {
    auto x = static_cast<std::size_t>(field.geometry->ix[i].x());
    auto y = static_cast<std::size_t>(field.geometry->ix[i].y());
    auto val = static_cast<TiffDataType>(scaleFactor * conc[i]);
    tifValues[y][x] = val;
  }

  // write to TIFF
  TIFF* tif = TIFFOpen(filename.c_str(), "w");
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

  for (std::size_t y = 0; y < static_cast<std::size_t>(height); y++) {
    int ret = TIFFWriteScanline(tif, tifValues.at(y).data(),
                                static_cast<uint32_t>(y));
    if (ret != 1) {
      SPDLOG_ERROR("TIFFWriteScanline error on row {}", y);
    }
  }
  TIFFClose(tif);

  return maxConc;
}

TiffReader::TiffReader(const std::string& filename) {
  TIFF* tif = TIFFOpen(filename.c_str(), "r");
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
    std::size_t photometric = 0;

    TIFFGetField(tif, TIFFTAG_IMAGEWIDTH, &width);
    TIFFGetField(tif, TIFFTAG_IMAGELENGTH, &height);
    TIFFGetField(tif, TIFFTAG_SAMPLESPERPIXEL, &samplespp);
    TIFFGetField(tif, TIFFTAG_BITSPERSAMPLE, &bitspp);
    TIFFGetField(tif, TIFFTAG_PHOTOMETRIC, &photometric);
    SPDLOG_DEBUG("  - {}x{} image", width, height);
    SPDLOG_DEBUG("    - {} samples per pixel", samplespp);
    SPDLOG_DEBUG("    - {} bits per sample", bitspp);
    SPDLOG_DEBUG("    - photometric enum: {}", photometric);

    if (samplespp == 1 && bitspp == TiffDataTypeBits &&
        photometric == PHOTOMETRIC_MINISBLACK) {
      SPDLOG_INFO("    --> importing {}bit grayscale image...", bitspp);
      auto& img = imgs.emplace_back();
      img.width = width;
      img.height = height;
      img.values = std::vector<std::vector<TiffDataType>>(
          height, std::vector<TiffDataType>(width, 0));
      for (std::size_t y = 0; y < height; y++) {
        TIFFReadScanline(tif, img.values.at(y).data(),
                         static_cast<uint32_t>(y));
        img.maxValue = std::max(*std::max_element(img.values.at(y).cbegin(),
                                                  img.values.at(y).cend()),
                                img.maxValue);
      }
    }
  } while (TIFFReadDirectory(tif) != 0);
  TIFFClose(tif);
}

std::size_t TiffReader::size() const { return imgs.size(); }

QImage TiffReader::getImage(std::size_t i) const {
  const auto& img = imgs.at(i);
  QImage image(static_cast<int>(img.width), static_cast<int>(img.height),
               QImage::Format_ARGB32_Premultiplied);
  for (int x = 0; x < image.width(); ++x) {
    for (int y = 0; y < image.height(); ++y) {
      // rescale pixel values from [0, max] to [0,255]
      unsigned int val16 = img.values.at(static_cast<std::size_t>(y))
                               .at(static_cast<std::size_t>(x));
      double intensity =
          static_cast<double>(val16) / static_cast<double>(img.maxValue);
      unsigned int val8 = 0x0000ff & static_cast<unsigned int>(256 * intensity);
      unsigned int rgb = 0xff000000 | val8 | (val8 << 8) | (val8 << 16);
      image.setPixel(x, y, rgb);
    }
  }
  return image;
}

}  // namespace utils
