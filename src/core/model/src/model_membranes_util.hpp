// Membrane utils

#pragma once

#include "geometry.hpp"
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

using QPointPair = std::pair<QPoint, QPoint>;

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
  std::vector<std::vector<QPointPair>> points;
  std::vector<std::vector<double>> weights;
  OrderedIntPairIndex colourIndexPairIndex;
  QVector<QRgb> colours;
  QSize imageSize{0, 0};

public:
  explicit ImageMembranePixels(const QImage &img, const QImage &pixelCorners);
  ~ImageMembranePixels();
  [[nodiscard]] int getColourIndex(QRgb colour) const;
  [[nodiscard]] const std::vector<QPointPair> *getPoints(int iA, int iB) const;
  [[nodiscard]] const std::vector<double> *getWeights(int iA, int iB) const;
  [[nodiscard]] const QSize &getImageSize() const;
};

} // namespace sme::model
