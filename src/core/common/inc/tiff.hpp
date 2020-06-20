// Wrapper around libTIFF
//  - writes field concentration as 16-bit grayscale tiff
//  - reads 16-bit grayscale tiff, including files with multiple images

#pragma once

#include <QImage>
#include <QString>
#include <cstdint>
#include <limits>
#include <string>
#include <vector>

namespace geometry {
class Field;
}

namespace utils {

using TiffDataType = uint16_t;

double writeTIFF(const std::string& filename, const QSize& imageSize,
                 const std::vector<double>& conc,
                 const std::vector<QPoint>& pixels, double pixelWidth);

class TiffReader {
 private:
  struct TiffImageData {
    std::vector<std::vector<double>> values;
    double maxValue = 0;
    double minValue = std::numeric_limits<double>::max();
    std::size_t width = 0;
    std::size_t height = 0;
  };
  std::vector<TiffImageData> tiffImages;
  QString errorMessage;

 public:
  explicit TiffReader(const std::string& filename);
  std::size_t size() const;
  QImage getImage(std::size_t i = 0) const;
  const QString& getErrorMessage() const;
};

}  // namespace utils
