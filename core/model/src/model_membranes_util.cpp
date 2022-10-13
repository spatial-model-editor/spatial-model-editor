#include "sme/model_membranes_util.hpp"
#include "sme/logger.hpp"
#include <QImage>
#include <QPoint>
#include <limits>
#include <memory>
#include <stdexcept>

namespace sme::model {

constexpr std::size_t nullIndex{std::numeric_limits<std::size_t>::max()};

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

ImageMembranePixels::ImageMembranePixels() = default;

ImageMembranePixels::ImageMembranePixels(const common::ImageStack &imgs) {
  setImages(imgs);
}

ImageMembranePixels::~ImageMembranePixels() = default;

void ImageMembranePixels::setImages(const common::ImageStack &imgs) {
  voxelPairs.clear();
  int nc{imgs[0].colorCount()};
  voxelPairs.resize(static_cast<std::size_t>(nc * (nc - 1)));
  colourIndexPairIndex = OrderedIntPairIndex{nc - 1};
  colours = imgs[0].colorTable();
  int nx{imgs[0].width()};
  int ny{imgs[0].height()};
  std::size_t nz{imgs.volume().depth()};
  imageSize = {nx, ny, nz};
  // for each pair of adjacent pixels of different colour,
  // add the pair of QPoints to the vector for this pair of colours
  for (std::size_t z = 0; z < nz; ++z) {
    const auto &img{imgs[z]};
    for (int y = 0; y < ny; ++y) {
      int prevIndex = img.pixelIndex(0, y);
      for (int x = 1; x < nx; ++x) {
        int currIndex = img.pixelIndex(x, y);
        if (currIndex < prevIndex) {
          auto i = colourIndexPairIndex.findOrInsert(currIndex, prevIndex);
          voxelPairs[i].push_back({{x, y, z}, {x - 1, y, z}});
        } else if (currIndex > prevIndex) {
          auto i = colourIndexPairIndex.findOrInsert(prevIndex, currIndex);
          voxelPairs[i].push_back({{x - 1, y, z}, {x, y, z}});
        }
        prevIndex = currIndex;
      }
    }
  }
  // y-neighbours
  for (std::size_t z = 0; z < nz; ++z) {
    const auto &img{imgs[z]};
    for (int x = 0; x < nx; ++x) {
      int prevIndex = img.pixelIndex(x, 0);
      for (int y = 1; y < ny; ++y) {
        int currIndex = img.pixelIndex(x, y);
        if (currIndex < prevIndex) {
          auto i = colourIndexPairIndex.findOrInsert(currIndex, prevIndex);
          voxelPairs[i].push_back({{x, y, z}, {x, y - 1, z}});
        } else if (currIndex > prevIndex) {
          auto i = colourIndexPairIndex.findOrInsert(prevIndex, currIndex);
          voxelPairs[i].push_back({{x, y - 1, z}, {x, y, z}});
        }
        prevIndex = currIndex;
      }
    }
  }
  // z-neighbours
  for (int x = 0; x < nx; ++x) {
    for (int y = 0; y < ny; ++y) {
      int prevIndex = imgs[0].pixelIndex(x, y);
      for (std::size_t z = 1; z < nz; ++z) {
        int currIndex = imgs[z].pixelIndex(x, y);
        if (currIndex < prevIndex) {
          auto i = colourIndexPairIndex.findOrInsert(currIndex, prevIndex);
          voxelPairs[i].push_back({{x, y, z}, {x, y, z - 1}});
        } else if (currIndex > prevIndex) {
          auto i = colourIndexPairIndex.findOrInsert(prevIndex, currIndex);
          voxelPairs[i].push_back({{x, y, z - 1}, {x, y, z}});
        }
        prevIndex = currIndex;
      }
    }
  }
}

int ImageMembranePixels::getColourIndex(QRgb colour) const {
  for (int i = 0; i < colours.size(); ++i) {
    if (colours[i] == colour) {
      return i;
    }
  }
  return -1;
}

const std::vector<VoxelPair> *ImageMembranePixels::getVoxels(int iA,
                                                             int iB) const {
  if (auto i = colourIndexPairIndex.find(iA, iB); i.has_value()) {
    return &voxelPairs[i.value()];
  }
  return nullptr;
}

const common::Volume &ImageMembranePixels::getImageSize() const {
  return imageSize;
}

} // namespace sme::model
