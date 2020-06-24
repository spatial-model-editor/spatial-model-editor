#include "geometry.hpp"

#include "logger.hpp"
#include "utils.hpp"

namespace geometry {

Compartment::Compartment(const std::string &compId, const QImage &img, QRgb col)
    : compartmentId{compId}, colour{col}, image{img.size(),
                                                QImage::Format_Mono} {
  image.setColor(0, qRgba(0, 0, 0, 0));
  image.setColor(1, col);
  image.fill(0);
  ix.clear();
  // find pixels in compartment: store image QPoint for each
  for (int x = 0; x < img.width(); ++x) {
    for (int y = 0; y < img.height(); ++y) {
      if (img.pixel(x, y) == col) {
        // if colour matches, add pixel to field
        QPoint p = QPoint(x, y);
        ix.push_back(p);
        image.setPixel(p, 1);
      }
    }
  }
  utils::QPointIndexer ixIndexer(img.size(), ix);
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

double Compartment::getPixelWidth() const { return pixelWidth; }

void Compartment::setPixelWidth(double width) { pixelWidth = width; }

const QImage &Compartment::getCompartmentImage() const { return image; }

Membrane::Membrane(const std::string &membraneId, const Compartment *A,
                   const Compartment *B,
                   const std::vector<std::pair<QPoint, QPoint>> *membranePairs)
    : id{membraneId}, compA{A}, compB{B},
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
  utils::QPointIndexer Aindexer(A->getCompartmentImage().size(),
                                A->getPixels());
  utils::QPointIndexer Bindexer(B->getCompartmentImage().size(),
                                B->getPixels());
  for (const auto &[pA, pB] : *membranePairs) {
    auto iA = Aindexer.getIndex(pA);
    auto iB = Bindexer.getIndex(pB);
    indexPair.push_back({iA.value(), iB.value()});
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

Field::Field(const Compartment *compartment, const std::string &specID,
             double diffConst, QRgb col)
    : id(specID), comp(compartment), diffusionConstant(diffConst), colour(col),
      conc(compartment->nPixels(), 0.0) {
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
  if (cmax < 1e-15) {
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
  const auto &img = comp->getCompartmentImage();
  int size = img.width() * img.height();
  // NOTE: order of concentration array is [ (x=0,y=0), (x=1,y=0), ... ]
  // NOTE: (0,0) point is at bottom-left
  // NOTE: QImage has (0,0) point at top-left, so flip y-coord here
  std::vector<double> arr(static_cast<std::size_t>(size), 0.0);
  for (std::size_t i = 0; i < comp->nPixels(); ++i) {
    const auto &point = comp->getPixel(i);
    int arrayIndex = point.x() + img.width() * (img.height() - 1 - point.y());
    arr[static_cast<std::size_t>(arrayIndex)] = conc[i];
  }
  return arr;
}

void Field::setCompartment(const Compartment *compartment) {
  SPDLOG_DEBUG("Changing compartment to {}", compartment->getId());
  comp = compartment;
  conc.assign(compartment->nPixels(), 0.0);
}

} // namespace geometry
