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

class ModelMembranesUtilError : public std::runtime_error {
  using std::runtime_error::runtime_error;
};

using VoxelPair = std::pair<common::Voxel, common::Voxel>;

class OrderedIntPairIndex {
private:
  int maxValue;
  std::size_t nItems{0};
  std::vector<std::size_t> values = {};
  [[nodiscard]] std::size_t toIndex(int smaller, int larger) const;

public:
  explicit OrderedIntPairIndex(int maxKeyValue = 4);
  void clear();
  std::size_t findOrInsert(int smaller, int larger);
  [[nodiscard]] std::optional<std::size_t> find(int smaller, int larger) const;
  [[nodiscard]] std::size_t size() const;
};

class ImageMembranePixels {
private:
  std::vector<std::vector<VoxelPair>> voxelPairs;
  OrderedIntPairIndex colorIndexPairIndex;
  QVector<QRgb> colors;
  common::Volume imageSize{};

public:
  explicit ImageMembranePixels();
  explicit ImageMembranePixels(const common::ImageStack &imgs);
  ~ImageMembranePixels();
  void setImages(const common::ImageStack &imgs);
  [[nodiscard]] int getColorIndex(QRgb color) const;
  void updateColor(QRgb oldColor, QRgb newColor);
  [[nodiscard]] const std::vector<VoxelPair> *getVoxels(int iA, int iB) const;
  [[nodiscard]] const common::Volume &getImageSize() const;
};

} // namespace sme::model
