// Wrapper around libTIFF
//  - writes field concentration as 16-bit grayscale tiff
//  - reads 16-bit grayscale tiff, including files with multiple images

#pragma once

#include "sme/image_stack.hpp"
#include "sme/voxel.hpp"
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
                 const std::vector<double> &conc,
                 const sme::common::VolumeF &voxelSize);

class TiffReader {
private:
  sme::common::ImageStack imageStack{};
  QString errorMessage{};

public:
  explicit TiffReader(const std::string &filename);
  [[nodiscard]] bool empty() const;
  [[nodiscard]] sme::common::ImageStack getImages() const;
  [[nodiscard]] const QString &getErrorMessage() const;
};

} // namespace sme::common
