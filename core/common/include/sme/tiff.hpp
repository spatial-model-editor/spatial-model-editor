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

/**
 * @brief Scalar sample type used when exporting TIFF concentration images.
 */
using TiffDataType = uint16_t;

/**
 * @brief Write a concentration field to a grayscale TIFF.
 *
 * @param filename Output TIFF filename.
 * @param imageSize Image dimensions.
 * @param conc Concentration values in image order.
 * @param voxelSize Physical voxel size.
 * @param physicalOrigin Physical location of the (0,0,0) pixel.
 * @returns Maximum concentration value used to scale TIFF pixel values, or a
 * negative value on failure.
 */
double writeTIFF(const std::string &filename, const QSize &imageSize,
                 const std::vector<double> &conc,
                 const sme::common::VolumeF &voxelSize,
                 const sme::common::VoxelF &physicalOrigin);

/**
 * @brief Reader for single- or multi-page TIFF images.
 */
class TiffReader {
private:
  sme::common::ImageStack imageStack{};
  QString errorMessage{};

public:
  /**
   * @brief Load image stack from TIFF file.
   * @param filename TIFF filename.
   */
  explicit TiffReader(const std::string &filename);
  /**
   * @brief Returns ``true`` if no images were loaded.
   * @returns Empty flag.
   */
  [[nodiscard]] bool empty() const;
  /**
   * @brief Get loaded images as an ``ImageStack``.
   * @returns Image stack.
   */
  [[nodiscard]] sme::common::ImageStack getImages() const;
  /**
   * @brief Error message from the last load attempt.
   * @returns Error message.
   */
  [[nodiscard]] const QString &getErrorMessage() const;
};

} // namespace sme::common
