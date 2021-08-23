#include "geometry.hpp"
#include "logger.hpp"
#include "utils.hpp"
#include <algorithm>
#include <initializer_list>
#include <limits>
#include <optional>
#include <stdexcept>
#include <utility>

namespace sme::geometry {

static void fillMissingByDilation(std::vector<std::size_t> &arr, int w, int h,
                                  std::size_t invalidIndex) {
  std::vector<std::size_t> arr_next{arr};
  const int maxIter{w + h};
  for (int iter = 0; iter < maxIter; ++iter) {
    bool finished{true};
    for (int y = 0; y < h; ++y) {
      for (int x = 0; x < w; ++x) {
        auto i{static_cast<std::size_t>(x + w * y)};
        if (arr[i] == invalidIndex) {
          // replace negative pixel with any valid 4-connected neighbour
          if (x > 0 && arr[i - 1] != invalidIndex) {
            arr_next[i] = arr[i - 1];
          } else if (x + 1 < w && arr[i + 1] != invalidIndex) {
            arr_next[i] = arr[i + 1];
          } else if (y > 0 &&
                     arr[i - static_cast<std::size_t>(w)] != invalidIndex) {
            arr_next[i] = arr[i - static_cast<std::size_t>(w)];
          } else if (y + 1 < h &&
                     arr[i + static_cast<std::size_t>(w)] != invalidIndex) {
            arr_next[i] = arr[i + static_cast<std::size_t>(w)];
          } else {
            // pixel has no valid neighbour: need another iteration
            finished = false;
          }
        }
      }
    }
    arr = arr_next;
    if (finished) {
      return;
    }
  }
  SPDLOG_WARN("Failed to replace all invalid pixels");
}

#if SPDLOG_ACTIVE_LEVEL <= SPDLOG_LEVEL_TRACE
static void
saveDebuggingIndicesImage(const std::vector<std::size_t> &arrayPoints,
                          const QSize &sz, std::size_t maxIndex,
                          const QString &filename) {
  auto norm{static_cast<float>(maxIndex)};
  QImage img(sz, QImage::Format_ARGB32_Premultiplied);
  img.fill(qRgba(0, 0, 0, 0));
  QColor c;
  for (int x = 0; x < sz.width(); ++x) {
    for (int y = 0; y < sz.height(); ++y) {
      auto i{arrayPoints[static_cast<std::size_t>(x + sz.width() * y)]};
      if (i <= maxIndex) {
        auto v{static_cast<float>(i) / norm};
        c.setHslF(1.0f - v, 1.0, 0.5f * v);
        img.setPixel(x, y, c.rgb());
      }
    }
  }
  img.save(filename);
}
#endif

Compartment::Compartment(std::string compId, const QImage &img, QRgb col)
    : compartmentId{std::move(compId)}, colour{col}, image{
                                                         img.size(),
                                                         QImage::Format_Mono} {
  image.setColor(0, qRgba(0, 0, 0, 0));
  image.setColor(1, col);
  image.fill(0);
  constexpr std::size_t invalidIndex{std::numeric_limits<std::size_t>::max()};
  arrayPoints.resize(static_cast<std::size_t>(img.width() * img.height()),
                     invalidIndex);
  std::size_t ixIndex{0};
  // find pixels in compartment: store image QPoint for each
  for (int x = 0; x < img.width(); ++x) {
    for (int y = 0; y < img.height(); ++y) {
      if (img.pixel(x, y) == col) {
        // if colour matches, add pixel to field
        QPoint p = QPoint(x, y);
        ix.push_back(p);
        image.setPixel(p, 1);
        // NOTE: (0,0) point in ix is at bottom-left, want top-left for array
        arrayPoints[static_cast<std::size_t>(
            x + img.width() * (img.height() - 1 - y))] = ixIndex++;
      }
    }
  }

#if SPDLOG_ACTIVE_LEVEL <= SPDLOG_LEVEL_TRACE
  saveDebuggingIndicesImage(arrayPoints, img.size(), ixIndex,
                            QString(compartmentId.c_str()) + "_indices.png");
#endif

  // for pixels outside compartment, find nearest pixel in compartment
  if (!ix.empty()) {
    fillMissingByDilation(arrayPoints, img.width(), img.height(), invalidIndex);
  }

#if SPDLOG_ACTIVE_LEVEL <= SPDLOG_LEVEL_TRACE
  saveDebuggingIndicesImage(arrayPoints, img.size(), ixIndex,
                            QString(compartmentId.c_str()) +
                                "_indices_dilated.png");
#endif

  common::QPointIndexer ixIndexer(img.size(), ix);
  // find nearest neighbours of each point
  nn.clear();
  nn.reserve(4 * ix.size());
  // find neighbours of each pixel in compartment
  for (std::size_t i = 0; i < ix.size(); ++i) {
    const QPoint &p = ix[i];
    for (const auto &pp :
         {QPoint(p.x() + 1, p.y()), QPoint(p.x() - 1, p.y()),
          QPoint(p.x(), p.y() + 1), QPoint(p.x(), p.y() - 1)}) {
      auto index = ixIndexer.getIndex(pp);
      if (index) {
        // neighbour of p is in same compartment
        nn.push_back(index.value());
      } else {
        // neighbour of p is outside compartment
        // Neumann zero flux bcs: set external neighbour of p to itself
        nn.push_back(i);
      }
    }
  }
  SPDLOG_INFO("compartmentId: {}", compartmentId);
  SPDLOG_INFO("n_pixels: {}", ix.size());
  SPDLOG_INFO("colour: {:x}", col);
}

const std::string &Compartment::getId() const { return compartmentId; }

QRgb Compartment::getColour() const { return colour; }

const QImage &Compartment::getCompartmentImage() const { return image; }

const std::vector<std::size_t> &Compartment::getArrayPoints() const {
  return arrayPoints;
}

Membrane::Membrane(std::string membraneId, const Compartment *A,
                   const Compartment *B,
                   const std::vector<std::pair<QPoint, QPoint>> *membranePairs)
    : id{std::move(membraneId)}, compA{A}, compB{B},
      image{A->getCompartmentImage().size(),
            QImage::Format_ARGB32_Premultiplied},
      pointPairs{membranePairs} {
  SPDLOG_INFO("membraneID: {}", id);
  SPDLOG_INFO("compartment A: {}", compA->getId());
  QRgb colA = A->getColour();
  SPDLOG_INFO("  - colour: {:x}", colA);
  SPDLOG_INFO("compartment B: {}", compB->getId());
  QRgb colB = B->getColour();
  SPDLOG_INFO("  - colour: {:x}", colB);
  SPDLOG_INFO("number of point pairs: {}", membranePairs->size());
  // convert each pair of QPoints into a pair of indices of the corresponding
  // points in the two compartments
  indexPair.clear();
  indexPair.reserve(membranePairs->size());
  common::QPointIndexer Aindexer(A->getCompartmentImage().size(),
                                 A->getPixels());
  common::QPointIndexer Bindexer(B->getCompartmentImage().size(),
                                 B->getPixels());
  for (const auto &[pA, pB] : *membranePairs) {
    auto iA = Aindexer.getIndex(pA);
    auto iB = Bindexer.getIndex(pB);
    indexPair.emplace_back(iA.value(), iB.value());
  }
  image.fill(qRgba(0, 0, 0, 0));
  for (const auto &[pA, pB] : *pointPairs) {
    image.setPixel(pA, colA);
    image.setPixel(pB, colB);
  }
}

const std::string &Membrane::getId() const { return id; }

void Membrane::setId(const std::string &membraneId) { id = membraneId; }

const Compartment *Membrane::getCompartmentA() const { return compA; }

const Compartment *Membrane::getCompartmentB() const { return compB; }

std::vector<std::pair<std::size_t, std::size_t>> &Membrane::getIndexPairs() {
  return indexPair;
}

const std::vector<std::pair<std::size_t, std::size_t>> &
Membrane::getIndexPairs() const {
  return indexPair;
}

const QImage &Membrane::getImage() const { return image; }

Field::Field(const Compartment *compartment, std::string specID,
             double diffConst, QRgb col)
    : id(std::move(specID)), comp(compartment), diffusionConstant(diffConst),
      colour(col), conc(compartment->nPixels(), 0.0) {
  SPDLOG_INFO("speciesID: {}", id);
  SPDLOG_INFO("compartmentID: {}", comp->getId());
}

const std::string &Field::getId() const { return id; }

QRgb Field::getColour() const { return colour; }

void Field::setColour(QRgb col) { colour = col; }

bool Field::getIsSpatial() const { return isSpatial; }

void Field::setIsSpatial(bool spatial) { isSpatial = spatial; }

bool Field::getIsUniformConcentration() const { return isUniformConcentration; }

void Field::setIsUniformConcentration(bool uniform) {
  isUniformConcentration = uniform;
}

double Field::getDiffusionConstant() const { return diffusionConstant; }

void Field::setDiffusionConstant(double diffConst) {
  diffusionConstant = diffConst;
}

const Compartment *Field::getCompartment() const { return comp; }

const std::vector<double> &Field::getConcentration() const { return conc; }

void Field::setConcentration(std::size_t index, double concentration) {
  conc[index] = concentration;
}

void Field::importConcentration(
    const std::vector<double> &sbmlConcentrationArray) {
  SPDLOG_INFO("species {}, compartment {}", id, comp->getId());
  SPDLOG_INFO("  - field has size {}", conc.size());
  SPDLOG_INFO("  - importing from sbml array of size {}",
              sbmlConcentrationArray.size());
  const auto &img = comp->getCompartmentImage();
  if (static_cast<int>(sbmlConcentrationArray.size()) !=
      img.width() * img.height()) {
    SPDLOG_ERROR("  - mismatch between array size [{}]"
                 " and compartment image size [{}x{} = {}]",
                 sbmlConcentrationArray.size(), img.width(), img.height(),
                 img.width() * img.height());
    throw std::invalid_argument("invalid array size");
  }
  // NOTE: order of concentration array is [ (x=0,y=0), (x=1,y=0), ... ]
  // NOTE: (0,0) point is at bottom-left
  // NOTE: QImage has (0,0) point at top-left, so flip y-coord here
  for (std::size_t i = 0; i < comp->nPixels(); ++i) {
    const auto &point = comp->getPixel(i);
    int arrayIndex = point.x() + img.width() * (img.height() - 1 - point.y());
    conc[i] = sbmlConcentrationArray[static_cast<std::size_t>(arrayIndex)];
  }
  isUniformConcentration = false;
}

void Field::setConcentration(const std::vector<double> &concentration) {
  conc = concentration;
}

void Field::setUniformConcentration(double concentration) {
  SPDLOG_INFO("species {}, compartment {}", id, comp->getId());
  SPDLOG_INFO("  - concentration = {}", concentration);
  std::fill(conc.begin(), conc.end(), concentration);
  isUniformConcentration = true;
}

QImage Field::getConcentrationImage() const {
  auto img = QImage(comp->getCompartmentImage().size(),
                    QImage::Format_ARGB32_Premultiplied);
  img.fill(qRgba(0, 0, 0, 0));
  // for now rescale conc to [0,1] to multiply species colour
  double cmax = *std::max_element(conc.cbegin(), conc.cend());
  constexpr double concMinNonZeroThreshold{1e-15};
  if (cmax < concMinNonZeroThreshold) {
    cmax = 1.0;
  }
  for (std::size_t i = 0; i < comp->nPixels(); ++i) {
    double scale = conc[i] / cmax;
    int r = static_cast<int>(scale * qRed(colour));
    int g = static_cast<int>(scale * qGreen(colour));
    int b = static_cast<int>(scale * qBlue(colour));
    img.setPixel(comp->getPixel(i), qRgb(r, g, b));
  }
  return img;
}

std::vector<double> Field::getConcentrationImageArray() const {
  std::vector<double> a;
  const auto &img = comp->getCompartmentImage();
  a.reserve(static_cast<std::size_t>(img.width() * img.height()));
  for (std::size_t i : comp->getArrayPoints()) {
    a.push_back(conc[i]);
  }
  return a;
}

void Field::setCompartment(const Compartment *compartment) {
  SPDLOG_DEBUG("Changing compartment to {}", compartment->getId());
  comp = compartment;
  conc.assign(compartment->nPixels(), 0.0);
}

} // namespace sme::geometry
