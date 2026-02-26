// Membrane utils

#pragma once

#include "sme/geometry.hpp"
#include <QPoint>
#include <QRgb>
#include <QSize>
#include <QVector>
#include <algorithm>
#include <cstddef>
#include <optional>
#include <utility>
#include <vector>

class QImage;

namespace libsbml {
class Model;
}

namespace sme::model {

/**
 * @brief Exception type for membrane utility failures.
 */
class ModelMembranesUtilError : public std::runtime_error {
  using std::runtime_error::runtime_error;
};

/**
 * @brief Pair of adjacent voxels across a membrane.
 */
using VoxelPair = std::pair<common::Voxel, common::Voxel>;

/**
 * @brief Dense index map keyed by ordered integer pairs.
 */
class OrderedIntPairIndex {
private:
  int maxValue;
  std::size_t nItems{0};
  std::vector<std::size_t> values = {};
  [[nodiscard]] std::size_t toIndex(int smaller, int larger) const;

public:
  /**
   * @brief Construct index map with expected max key value.
   */
  explicit OrderedIntPairIndex(int maxKeyValue = 4);
  /**
   * @brief Clear all entries.
   */
  void clear();
  /**
   * @brief Lookup pair index or insert if missing.
   */
  std::size_t findOrInsert(int smaller, int larger);
  /**
   * @brief Lookup pair index.
   */
  [[nodiscard]] std::optional<std::size_t> find(int smaller, int larger) const;
  /**
   * @brief Number of stored pairs.
   */
  [[nodiscard]] std::size_t size() const;
};

/**
 * @brief Membrane voxel-pair extraction from segmented images.
 */
class ImageMembranePixels {
private:
  std::vector<std::vector<VoxelPair>> voxelPairs;
  OrderedIntPairIndex colorIndexPairIndex;
  QVector<QRgb> colors;
  common::Volume imageSize{};

public:
  /**
   * @brief Construct empty membrane-pixel map.
   */
  explicit ImageMembranePixels();
  /**
   * @brief Construct membrane-pixel map from geometry images.
   */
  explicit ImageMembranePixels(const common::ImageStack &imgs);
  /**
   * @brief Destructor.
   */
  ~ImageMembranePixels();
  /**
   * @brief Recompute membrane-pixel map from geometry images.
   */
  void setImages(const common::ImageStack &imgs);
  /**
   * @brief Color-table index for a color, or ``-1`` if absent.
   */
  [[nodiscard]] int getColorIndex(QRgb color) const;
  /**
   * @brief Update color entries in extracted membrane data.
   */
  void updateColor(QRgb oldColor, QRgb newColor);
  /**
   * @brief Adjacent voxel pairs for the color-index pair.
   */
  [[nodiscard]] const std::vector<VoxelPair> *getVoxels(int iA, int iB) const;
  /**
   * @brief Geometry image size used to build the mapping.
   */
  [[nodiscard]] const common::Volume &getImageSize() const;
};

} // namespace sme::model
