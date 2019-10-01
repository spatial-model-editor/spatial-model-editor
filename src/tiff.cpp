#include "tiff.hpp"

#include "logger.hpp"
#include "tiffio.h"
#include "utils.hpp"

namespace utils {

using TiffDataType = uint16_t;
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
  TIFFSetField(tif, TIFFTAG_IMAGEWIDTH, width);
  TIFFSetField(tif, TIFFTAG_IMAGELENGTH, height);
  // 16-bit grayscale - one uint16_t per pixel:
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
  // NB: location of (0,0) pixel (ignored by DUNE):
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

}  // namespace utils
