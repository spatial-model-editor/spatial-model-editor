// Wrapper around libTIFF
//  - writes field concentration as 16-bit grayscale tiff
//  - reads 16-bit grayscale tiff, including files with multiple images

#pragma once

#include <QImage>
#include <QString>
#include <cstddef>
#include <cstdint>
#include <limits>
#include <string>
#include <vector>

class QSize;
class QPoint;

namespace sme::common {

using TiffDataType = uint16_t;

double writeTIFF(const std::string &filename, const QSize &imageSize,
                 const std::vector<double> &conc, double pixelWidth);

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
  explicit TiffReader(const std::string &filename);
  [[nodiscard]] std::size_t size() const;
  [[nodiscard]] QImage getImage(std::size_t i = 0) const;
  [[nodiscard]] const QString &getErrorMessage() const;
};

} // namespace sme::common
