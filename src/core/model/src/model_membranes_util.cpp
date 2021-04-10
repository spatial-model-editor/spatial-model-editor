#include "model_membranes_util.hpp"
#include <QImage>
#include <QPoint>
#include <limits>
#include <memory>
#include <stdexcept>

namespace sme::model {

constexpr std::size_t nullIndex = std::numeric_limits<std::size_t>::max();

std::size_t OrderedIntPairIndex::toIndex(int smaller, int larger) const {
  auto index = static_cast<std::size_t>(smaller + maxValue * larger);
  if (smaller >= larger) {
    throw(std::runtime_error(
        "OrderedIntPairIndex :: incorrectly ordered pair of ints"));
  }
  if (index >= values.size()) {
    throw(std::runtime_error(
        "OrderedIntPairIndex :: invalid ints (probably too large)"));
  }
  return index;
}

OrderedIntPairIndex::OrderedIntPairIndex(int maxKeyValue)
    : maxValue{maxKeyValue} {
  values.assign(static_cast<std::size_t>((maxValue + 1) * maxValue), nullIndex);
}

void OrderedIntPairIndex::clear() {
  std::fill(values.begin(), values.end(), nullIndex);
  nItems = 0;
}

std::size_t OrderedIntPairIndex::findOrInsert(int smaller, int larger) {
  auto index = toIndex(smaller, larger);
  auto &i = values[index];
  if (i == nullIndex) {
    i = nItems;
    ++nItems;
  }
  return i;
}

std::optional<std::size_t> OrderedIntPairIndex::find(int smaller,
                                                     int larger) const {
  auto ix = values[toIndex(smaller, larger)];
  if (ix == nullIndex) {
    return {};
  }
  return ix;
}

std::size_t OrderedIntPairIndex::size() const { return nItems; }

// relative length of a pixel edge between vertices v1, v2
// see 3.B.(ii) of https://doi.org/10.1016/S0146-664X(79)80042-8
static double getEdgeWeight(QRgb v1, QRgb v2) {
  constexpr double baseWeight{0.948};
  constexpr double cornerWeight{0.139};
  constexpr QRgb cornerColour{qRgb(255, 0, 0)};
  auto nCorners{static_cast<int>(v1 == cornerColour) +
                static_cast<int>(v2 == cornerColour)};
  return baseWeight - static_cast<double>(nCorners) * cornerWeight;
}

ImageMembranePixels::ImageMembranePixels(const QImage &img,
                                         const QImage &pixelCorners) {
  points.clear();
  weights.clear();
  points.resize(
      static_cast<std::size_t>(img.colorCount() * (img.colorCount() - 1)));
  weights.resize(points.size());
  colourIndexPairIndex = OrderedIntPairIndex{img.colorCount() - 1};
  colours = img.colorTable();
  imageSize = img.size();
  // for each pair of adjacent pixels of different colour,
  // add the pair of QPoints to the vector for this pair of colours
  for (int y = 0; y < img.height(); ++y) {
    int prevIndex = img.pixelIndex(0, y);
    for (int x = 1; x < img.width(); ++x) {
      int currIndex = img.pixelIndex(x, y);
      if (currIndex != prevIndex) {
        auto weight{getEdgeWeight(pixelCorners.pixel(x, y),
                                  pixelCorners.pixel(x, y + 1))};
        if (currIndex < prevIndex) {
          auto i = colourIndexPairIndex.findOrInsert(currIndex, prevIndex);
          points[i].push_back({QPoint(x, y), QPoint(x - 1, y)});
          weights[i].push_back(weight);
        } else if (currIndex > prevIndex) {
          auto i = colourIndexPairIndex.findOrInsert(prevIndex, currIndex);
          points[i].push_back({QPoint(x - 1, y), QPoint(x, y)});
          weights[i].push_back(weight);
        }
      }
      prevIndex = currIndex;
    }
  }
  // y-neighbours
  for (int x = 0; x < img.width(); ++x) {
    int prevIndex = img.pixelIndex(x, 0);
    for (int y = 1; y < img.height(); ++y) {
      int currIndex = img.pixelIndex(x, y);
      if (currIndex != prevIndex) {
        auto weight{getEdgeWeight(pixelCorners.pixel(x, y),
                                  pixelCorners.pixel(x + 1, y))};
        if (currIndex < prevIndex) {
          auto i = colourIndexPairIndex.findOrInsert(currIndex, prevIndex);
          points[i].push_back({QPoint(x, y), QPoint(x, y - 1)});
          weights[i].push_back(weight);
        } else if (currIndex > prevIndex) {
          auto i = colourIndexPairIndex.findOrInsert(prevIndex, currIndex);
          points[i].push_back({QPoint(x, y - 1), QPoint(x, y)});
          weights[i].push_back(weight);
        }
      }
      prevIndex = currIndex;
    }
  }
}

ImageMembranePixels::~ImageMembranePixels() = default;

int ImageMembranePixels::getColourIndex(QRgb colour) const {
  for (int i = 0; i < colours.size(); ++i) {
    if (colours[i] == colour) {
      return i;
    }
  }
  return -1;
}

const std::vector<QPointPair> *ImageMembranePixels::getPoints(int iA,
                                                              int iB) const {
  if (auto i = colourIndexPairIndex.find(iA, iB); i.has_value()) {
    return &points[i.value()];
  }
  return nullptr;
}

[[nodiscard]] const std::vector<double> *
ImageMembranePixels::getWeights(int iA, int iB) const {
  if (auto i = colourIndexPairIndex.find(iA, iB); i.has_value()) {
    return &weights[i.value()];
  }
  return nullptr;
}

const QSize &ImageMembranePixels::getImageSize() const { return imageSize; }

} // namespace sme::model
