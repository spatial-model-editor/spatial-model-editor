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

using VoxelPair = std::pair<geometry::Voxel, geometry::Voxel>;

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
  OrderedIntPairIndex colourIndexPairIndex;
  QVector<QRgb> colours;
  geometry::VSize imageSize{};

public:
  explicit ImageMembranePixels();
  explicit ImageMembranePixels(const std::vector<QImage> &imgs);
  ~ImageMembranePixels();
  void setImages(const std::vector<QImage> &imgs);
  [[nodiscard]] int getColourIndex(QRgb colour) const;
  [[nodiscard]] const std::vector<VoxelPair> *getVoxels(int iA, int iB) const;
  [[nodiscard]] const geometry::VSize &getImageSize() const;
};

} // namespace sme::model
