#pragma once

#include "sme/voxel.hpp"
#include <QImage>
#include <QString>

namespace sme::common {

/**
 * @brief Stack of 2d QImages
 *
 * A 3d image stored as a stack of z-slices, one QImage per slice.
 */
class ImageStack {
private:
  std::vector<QImage> imgs{};
  Volume sz{0, 0, 0};
  VolumeF voxel{1.0, 1.0, 1.0};

public:
  /**
   * @brief Construct an empty image stack.
   */
  ImageStack();
  /**
   * @brief Construct ``depth`` images of a given size and format.
   * @param size Image stack size.
   * @param format QImage format for each slice.
   */
  ImageStack(const Volume &size, QImage::Format format);
  /**
   * @brief Construct from image slices, moving ownership.
   * @param images Slice images.
   */
  explicit ImageStack(std::vector<QImage> &&images);
  /**
   * @brief Construct from image slices by copy.
   * @param images Slice images.
   */
  explicit ImageStack(const std::vector<QImage> &images);
  /**
   * @brief Load image stack from file.
   *
   * Attempts TIFF import first, then falls back to ``QImage::load``.
   * @param filename Image filename.
   */
  explicit ImageStack(const QString &filename);
  /**
   * @brief Indexed format used for palette-based stacks.
   */
  static constexpr auto indexedImageFormat = QImage::Format_Indexed8;
  /**
   * @brief Conversion flags used when converting to indexed images.
   */
  static constexpr auto indexedImageConversionFlags =
      Qt::AvoidDither | Qt::ThresholdDither | Qt::ThresholdAlphaDither |
      Qt::NoOpaqueDetection;
  /**
   * @brief Grayscale image from array of pixel intensities
   *
   * If a positive value is supplied for `maxValue` it determines the value that
   * corresponds to white in the image, otherwise by default the maximum value
   * from `values` is used.
   * @param imageSize Output image size.
   * @param values Scalar values to map to grayscale.
   * @param maxValue Maximum value for white; negative means auto-detect.
   */
  ImageStack(const Volume &imageSize, const std::vector<double> &values,
             double maxValue = -1.0);
  /**
   * @brief Mutable access to z slice.
   * @param z Slice index.
   * @returns Mutable slice reference.
   */
  inline QImage &operator[](std::size_t z) { return imgs[z]; }
  /**
   * @brief Read-only access to z slice.
   * @param z Slice index.
   * @returns Immutable slice reference.
   */
  [[nodiscard]] inline const QImage &operator[](std::size_t z) const {
    return imgs[z];
  }
  /**
   * @brief Returns ``true`` if stack has no slices.
   * @returns Empty flag.
   */
  [[nodiscard]] inline bool empty() const { return imgs.empty(); }
  /**
   * @brief Color table of the first slice (or empty if stack is empty).
   * @returns Slice color table.
   */
  [[nodiscard]] QList<QRgb> colorTable() const;
  /**
   * @brief Stack size in voxels.
   * @returns Stack volume.
   */
  [[nodiscard]] inline const Volume &volume() const noexcept { return sz; }
  /**
   * @brief Physical voxel size.
   * @returns Voxel size.
   */
  [[nodiscard]] inline const VolumeF &voxelSize() const noexcept {
    return voxel;
  }
  /**
   * @brief Set physical voxel size.
   * @param voxelSize New voxel size.
   */
  inline void setVoxelSize(const VolumeF &voxelSize) noexcept {
    voxel = voxelSize;
  }
  /**
   * @brief Iterator to first slice.
   */
  inline std::vector<QImage>::iterator begin() noexcept { return imgs.begin(); }
  /**
   * @brief Const iterator to first slice.
   */
  [[nodiscard]] inline std::vector<QImage>::const_iterator
  begin() const noexcept {
    return imgs.cbegin();
  }
  /**
   * @brief Iterator past last slice.
   */
  inline std::vector<QImage>::iterator end() noexcept { return imgs.end(); }
  /**
   * @brief Const iterator past last slice.
   */
  [[nodiscard]] inline std::vector<QImage>::const_iterator
  end() const noexcept {
    return imgs.cend();
  }
  /**
   * @brief Remove all slices.
   */
  void clear();
  /**
   * @brief Fill every pixel in every slice with ``pixel``.
   * @param pixel Pixel value.
   */
  void fill(uint pixel);
  /**
   * @brief Resize all slices to ``size``.
   * @param size New slice size.
   */
  void rescaleXY(QSize size);
  /**
   * @brief Flip all slices vertically.
   */
  void flipYAxis();
  /**
   * @brief Convert all slices to a shared indexed color format.
   */
  void convertToIndexed();
  /**
   * @brief Set palette entry ``i`` to color ``c`` for all slices.
   * @param i Palette index.
   * @param c New color.
   */
  void setColor(int i, QRgb c);
  /**
   * @brief Check whether a voxel coordinate is inside this stack.
   * @param voxel Voxel coordinate.
   * @returns ``true`` if voxel is inside bounds.
   */
  [[nodiscard]] bool valid(const Voxel &voxel) const;
  /**
   * @brief Return resized copy with explicit width and height.
   * @param width Output width.
   * @param height Output height.
   * @returns Resized image stack.
   */
  ImageStack scaled(int width, int height);
  /**
   * @brief Return resized copy preserving aspect ratio by width.
   * @param width Output width.
   * @returns Resized image stack.
   */
  ImageStack scaledToWidth(int width);
  /**
   * @brief Return resized copy preserving aspect ratio by height.
   * @param height Output height.
   * @returns Resized image stack.
   */
  ImageStack scaledToHeight(int height);
};

} // namespace sme::common
