// Wrapper around libTIFF
//  - writes field concentration as 16-bit grayscale tiff
//  - reads 16-bit grayscale tiff, including files with multiple images

#pragma once

#include <QImage>
#include <string>
#include <vector>

namespace geometry {
class Field;
}

namespace utils {

using TiffDataType = uint16_t;

double writeTIFF(const std::string& filename, const geometry::Field& field,
                 double pixelWidth = 1.0);

class TiffReader {
 private:
  struct TiffImageData {
    std::vector<std::vector<TiffDataType>> values;
    TiffDataType maxValue = 0;
    std::size_t width = 0;
    std::size_t height = 0;
  };
  std::vector<TiffImageData> tiffImages;

 public:
  explicit TiffReader(const std::string& filename);
  std::size_t size() const;
  QImage getImage(std::size_t i = 0) const;
};

}  // namespace utils
