#include "boundary_pixels.hpp"

#include <array>
#include <optional>

#include "logger.hpp"
#include "utils.hpp"

namespace mesh {

static constexpr std::size_t nullIndex =
    std::numeric_limits<std::size_t>::max();

inline std::size_t BoundaryPixels::toIndex(const QPoint &point) const noexcept {
  // grid is 1 pixel larger than image on all sides
  auto w = boundaryPixelsImage.width() + 2;
  auto ix = point.x() + 1;
  auto iy = point.y() + 1;
  return static_cast<std::size_t>(ix + iy * w);
}

void BoundaryPixels::setPointColour(const QPoint &point, std::size_t value) {
  QRgb rgb = qRgb(255, 0, 0);  // fixed point colour
  if (value < fpIndexOffset) {
    rgb = pixelColours[value];
  }
  int x = point.x();
  int y = height() - point.y() - 1;
  boundaryPixelsImage.setPixel(x, y, rgb);
}

void BoundaryPixels::setBoundaryPoint(const QPoint &point, std::size_t value) {
  SPDLOG_TRACE("({},{}) -> {}", point.x(), point.y(), value);
  auto &v = values[toIndex(point)];
  if (v == nullIndex || value > v) {
    // only overwrite with higher value (i.e. FP overwrites boundary)
    v = value;
    setPointColour(point, value);
  }
}

void BoundaryPixels::setFixedPoint(const QPoint &point) {
  auto value = getOrInsertFixedPointIndex(point) + fpIndexOffset;
  // set all neighbouring pixels to point to this fixed point
  for (int x = point.x() - 1; x <= point.x() + 1; ++x) {
    for (int y = point.y() - 1; y <= point.y() + 1; ++y) {
      if (QPoint p(x, y); isValid(p)) {
        setBoundaryPoint(p, value);
      }
    }
  }
}

int BoundaryPixels::width() const { return boundaryPixelsImage.width(); }

int BoundaryPixels::height() const { return boundaryPixelsImage.height(); }

bool BoundaryPixels::isValid(const QPoint &point) const {
  auto x = static_cast<std::size_t>(point.x());
  auto y = static_cast<std::size_t>(point.y());
  auto w = static_cast<std::size_t>(boundaryPixelsImage.width());
  auto h = static_cast<std::size_t>(boundaryPixelsImage.height());
  return x < w && y < h;
}

bool BoundaryPixels::isBoundary(const QPoint &point) const {
  return values[toIndex(point)] != nullIndex;
}

bool BoundaryPixels::isFixed(const QPoint &point) const {
  return getFixedPointIndex(point) != nullIndex;
}

bool BoundaryPixels::isMembrane(const QPoint &point) const {
  return getMembraneIndex(point) != nullIndex;
}

std::size_t BoundaryPixels::getMembraneIndex(const QPoint &point) const {
  auto value = values[toIndex(point)];
  return value < membraneIndexOffset ? membraneIndices[value] : nullIndex;
}

const std::string &BoundaryPixels::getMembraneName(std::size_t i) const {
  if (i >= membraneNames.size()) {
    return defaultMembraneName;
  }
  return membraneNames[i];
}

std::size_t BoundaryPixels::getFixedPointIndex(const QPoint &point) const {
  auto value = values[toIndex(point)];
  if (value == nullIndex || value < fpIndexOffset) {
    return nullIndex;
  }
  return value - fpIndexOffset;
}

std::size_t BoundaryPixels::getOrInsertFixedPointIndex(const QPoint &point) {
  auto i = getFixedPointIndex(point);
  if (i == nullIndex) {
    i = fixedPoints.size();
    fixedPoints.push_back(point);
  }
  return i;
}

const QPoint &BoundaryPixels::getFixedPoint(const QPoint &point) const {
  return fixedPoints[getFixedPointIndex(point)];
}

std::optional<QPoint> BoundaryPixels::getNeighbourOnBoundary(
    const QPoint &point) const {
  constexpr std::array<QPoint, 8> nnp = {
      QPoint(1, 0), QPoint(-1, 0), QPoint(0, 1),  QPoint(0, -1),
      QPoint(1, 1), QPoint(1, -1), QPoint(-1, 1), QPoint(-1, -1)};
  auto value = values[toIndex(point)];
  for (const auto &dp : nnp) {
    if (QPoint newPoint = point + dp;
        isValid(newPoint) && values[toIndex(newPoint)] == value) {
      return newPoint;
    }
  }
  return {};
}

const std::vector<QPoint> &BoundaryPixels::getFixedPoints() const {
  return fixedPoints;
}

const QImage &BoundaryPixels::getBoundaryPixelsImage() const {
  return boundaryPixelsImage;
}

void BoundaryPixels::visitPoint(const QPoint &point) {
  auto &value = values[toIndex(point)];
  if (value < fpIndexOffset) {
    value = nullIndex;
  }
}

template <typename T>
static std::size_t findIndex(const std::vector<T> &vec, T val) {
  auto iter = std::find(vec.cbegin(), vec.cend(), val);
  if (iter == vec.cend()) {
    return nullIndex;
  }
  return static_cast<std::size_t>(std::distance(vec.cbegin(), iter));
}

static QRgb avgColours(const std::pair<QRgb, QRgb> &cols) {
  auto [c1, c2] = cols;
  return qRgb((qRed(c1) + qRed(c2)) / 2, (qGreen(c1) + qGreen(c2)) / 2,
              (qBlue(c1) + qBlue(c2)) / 2);
}

// map from flattened pair of compartment indices to membrane index
static std::tuple<std::vector<std::string>, std::vector<std::size_t>,
                  std::vector<QRgb>>
getMembraneVectors(
    const std::vector<std::pair<std::string, ColourPair>> &membraneColourPairs,
    const std::vector<QRgb> &compartments) {
  std::tuple<std::vector<std::string>, std::vector<std::size_t>,
             std::vector<QRgb>>
      tup;
  auto &[names, indices, colours] = tup;
  names.reserve(membraneColourPairs.size());
  std::size_t nC = compartments.size();
  indices = std::vector<std::size_t>(nC * nC, nullIndex);
  colours = std::vector<QRgb>(nC * (nC + 1), qRgba(0, 0, 0, 0));
  for (std::size_t i = 0; i < compartments.size(); ++i) {
    colours[nC * nC + i] = compartments[i];
  }
  for (const auto &[name, cols] : membraneColourPairs) {
    auto i1 = findIndex(compartments, cols.first);
    auto i2 = findIndex(compartments, cols.second);
    auto avgCol = avgColours(cols);
    for (auto i : {i1 + nC * i2, i2 + nC * i1}) {
      indices[i] = names.size();
      colours[i] = avgCol;
    }
    names.push_back(name);
  }
  return tup;
}

// get index of given Rgb colour in indexed image
static std::size_t getColourIndex(QRgb col, const QImage &img) {
  for (int i = 0; i < img.colorCount(); ++i) {
    if (col == img.color(i)) {
      return static_cast<std::size_t>(i);
    }
  }
  return nullIndex;
}

// map from img indexed colours to index in compartments vector
// indexed colours not present in cols vector set to nullIndex
static std::vector<std::size_t> getColourIndexToCompartmentIndex(
    const std::vector<QRgb> &compartmentColours, const QImage &img) {
  auto nImgColours = static_cast<std::size_t>(img.colorCount());
  SPDLOG_TRACE("{}-colour image", nImgColours);
  for (int i = 0; i < img.colorCount(); ++i) {
    SPDLOG_TRACE("  - {:x}", img.color(i));
  }
  std::vector<std::size_t> vec(nImgColours, nullIndex);
  std::size_t compIndex = 0;
  for (auto compColour : compartmentColours) {
    auto colIndex = getColourIndex(compColour, img);
    if (colIndex == nullIndex) {
      SPDLOG_ERROR("compColour {:x} gave null index in image", compColour);
    }
    vec[colIndex] = compIndex;
    SPDLOG_TRACE("comp[{}] : colour {:x}, index {}", compIndex, compColour,
                 colIndex);
    ++compIndex;
  }
  return vec;
}

namespace {
class PixelCompartments {
 private:
  QImage img;
  QPoint meshPoint;
  utils::SmallStackSet<std::size_t, 5> set;
  std::vector<std::size_t> compartments;
  std::size_t nCompartments;
  std::size_t pointToCompIndex(const QPoint &p) {
    return compartments[static_cast<std::size_t>(img.pixelIndex(p))];
  }

 public:
  void setPixel(const QPoint &point) {
    // want (0,0) point in bottom left for mesh
    meshPoint = QPoint(point.x(), img.height() - 1 - point.y());
    set.clear();
    set.insert(pointToCompIndex(point));
    for (const auto &dp :
         {QPoint(1, 0), QPoint(-1, 0), QPoint(0, 1), QPoint(0, -1)}) {
      QPoint nn = point + dp;
      if (img.valid(nn)) {
        set.insert(pointToCompIndex(nn));
      } else {
        set.insert(nullIndex);
      }
    }
  }
  std::size_t compartmentIndex() const { return set[0]; }
  std::size_t firstNeighbourIndex() const { return set[1]; }
  std::size_t membraneIndex() const {
    auto otherIndex = firstNeighbourIndex();
    if (otherIndex == nullIndex) {
      otherIndex = nCompartments;
    }
    return compartmentIndex() + nCompartments * otherIndex;
  }
  std::size_t neighbourCount() const { return set.size() - 1; }
  const QPoint &getMeshPoint() const { return meshPoint; }
  explicit PixelCompartments(const std::vector<QRgb> &compartmentColours,
                             const QImage &inputImage)
      : img{inputImage.convertToFormat(QImage::Format_Indexed8)},
        nCompartments{compartmentColours.size()} {
    compartments = getColourIndexToCompartmentIndex(compartmentColours, img);
    SPDLOG_TRACE("{}x{} image", img.width(), img.height());
  }
};

}  // namespace

BoundaryPixels::BoundaryPixels(
    const QImage &inputImage, const std::vector<QRgb> &compartmentColours,
    const std::vector<std::pair<std::string, ColourPair>> &membraneColourPairs)
    : boundaryPixelsImage(inputImage.size(),
                          QImage::Format_ARGB32_Premultiplied),
      values(static_cast<std::size_t>((inputImage.width() + 2) *
                                      (inputImage.height() + 2)),
             nullIndex),
      nCompartments{compartmentColours.size()},
      membraneIndexOffset{compartmentColours.size() *
                          compartmentColours.size()},
      fpIndexOffset{compartmentColours.size() *
                    (compartmentColours.size() + 1)} {
  boundaryPixelsImage.fill(qRgba(0, 0, 0, 0));
  std::tie(membraneNames, membraneIndices, pixelColours) =
      getMembraneVectors(membraneColourPairs, compartmentColours);
  auto pixel = PixelCompartments(compartmentColours, inputImage);
  SPDLOG_TRACE("{}x{} image", inputImage.width(), inputImage.height());
  for (int x = 0; x < inputImage.width(); ++x) {
    for (int y = 0; y < inputImage.height(); ++y) {
      pixel.setPixel(QPoint(x, y));
      if (pixel.neighbourCount() > 1) {
        // multiple neighbours -> fixed point
        setFixedPoint(pixel.getMeshPoint());
      } else if (pixel.neighbourCount() == 1 &&
                 pixel.compartmentIndex() < pixel.firstNeighbourIndex()) {
        // single neighbour with larger index -> boundary point
        setBoundaryPoint(pixel.getMeshPoint(), pixel.membraneIndex());
      }
    }
  }
#if SPDLOG_ACTIVE_LEVEL <= SPDLOG_LEVEL_TRACE
  inputImage.save("boundaryPixelsInputImage.png");
  boundaryPixelsImage.save("boundaryPixels.png");
#endif
}

}  // namespace mesh
