#include "sme/geometry.hpp"
#include "geometry_impl.hpp"
#include "sme/geometry_utils.hpp"
#include "sme/logger.hpp"
#include "sme/utils.hpp"
#include <algorithm>
#include <initializer_list>
#include <limits>
#include <stdexcept>
#include <utility>

using sme::common::Voxel;

namespace sme::geometry {

Compartment::Compartment(std::string compId, const common::ImageStack &imgs,
                         QRgb col)
    : compartmentId{std::move(compId)}, colour{col} {
  if (imgs.empty()) {
    return;
  }
  images = common::ImageStack(imgs.volume(), QImage::Format_Mono);
  int nx{images.volume().width()};
  int ny{images.volume().height()};
  int nz{static_cast<int>(images.volume().depth())};
  constexpr std::size_t invalidIndex{std::numeric_limits<std::size_t>::max()};
  arrayPoints.resize(images.volume().nVoxels(), invalidIndex);
  std::size_t ixIndex{0};
  // find voxels in compartment
  for (std::size_t z = 0; z < static_cast<std::size_t>(nz); ++z) {
    auto &image{images[z]};
    image.setColor(0, qRgba(0, 0, 0, 0));
    image.setColor(1, col);
    image.fill(0);
    for (int x = 0; x < nx; ++x) {
      for (int y = 0; y < ny; ++y) {
        if (imgs[z].pixel(x, y) == col) {
          // if colour matches, add voxel to field
          ix.emplace_back(x, y, z);
          image.setPixel(x, y, 1);
          // NOTE: y=0 in ix is at bottom of image,
          // but we want it at the top in arrayPoints, so y index is inverted
          arrayPoints[static_cast<std::size_t>(x + nx * (ny - 1 - y)) +
                      (static_cast<std::size_t>(nx * ny) * z)] = ixIndex++;
        }
      }
    }
  }

#if SPDLOG_ACTIVE_LEVEL <= SPDLOG_LEVEL_TRACE
  for (std::size_t iz = 0; iz < static_cast<std::size_t>(nz); ++iz) {
    saveDebuggingIndicesImageXY(arrayPoints, nx, ny, nz, ixIndex,
                                QString(compartmentId.c_str()) + "_indices");
  }
#endif

  // for voxels outside compartment, find nearest voxel within compartment
  if (!ix.empty()) {
    fillMissingByDilation(arrayPoints, nx, ny, nz, invalidIndex);
  }

#if SPDLOG_ACTIVE_LEVEL <= SPDLOG_LEVEL_TRACE
  saveDebuggingIndicesImageXY(arrayPoints, nx, ny, nz, ixIndex,
                              QString(compartmentId.c_str()) +
                                  "_indices_dilated");
#endif

  VoxelIndexer ixIndexer(nx, ny, nz, ix);
  // find nearest neighbours of each point
  nn.clear();
  nn.reserve(6 * ix.size());
  // find neighbours of each voxel in compartment
  for (std::size_t i = 0; i < ix.size(); ++i) {
    const auto &v{ix[i]};
    const auto x{v.p.x()};
    const auto y{v.p.y()};
    const auto z{v.z};
    for (const auto &vn :
         {Voxel{x + 1, y, z}, Voxel{x - 1, y, z}, Voxel{x, y + 1, z},
          Voxel{x, y - 1, z}, Voxel{x, y, z + 1}, Voxel{x, y, z - 1}}) {
      // nearest neighbour is index of voxel vn if vn is in the same compartment
      // as v, otherwise set to the index of v itself (Neumann zero flux bcs)
      nn.push_back(ixIndexer.getIndex(vn).value_or(i));
    }
  }
  SPDLOG_INFO("compartmentId: {}", compartmentId);
  SPDLOG_INFO("n_voxels: {}", ix.size());
  SPDLOG_INFO("colour: {:x}", col);
}

const std::string &Compartment::getId() const { return compartmentId; }

QRgb Compartment::getColour() const { return colour; }

const common::Volume &Compartment::getImageSize() const {
  return images.volume();
}

const common::ImageStack &Compartment::getCompartmentImages() const {
  return images;
}

const std::vector<std::size_t> &Compartment::getArrayPoints() const {
  return arrayPoints;
}

Membrane::Membrane(std::string membraneId, const Compartment *A,
                   const Compartment *B,
                   const std::vector<std::pair<Voxel, Voxel>> *voxelPairs)
    : id{std::move(membraneId)}, compA{A}, compB{B} {
  const auto &imageSize{A->getImageSize()};
  images = {imageSize, QImage::Format_ARGB32_Premultiplied};
  SPDLOG_INFO("membraneID: {}", id);
  SPDLOG_INFO("compartment A: {}", compA->getId());
  QRgb colA = A->getColour();
  SPDLOG_INFO("  - colour: {:x}", colA);
  SPDLOG_INFO("compartment B: {}", compB->getId());
  QRgb colB = B->getColour();
  SPDLOG_INFO("  - colour: {:x}", colB);
  SPDLOG_INFO("number of voxel pairs: {}", voxelPairs->size());

  // convert each pair of voxels into a pair of indices of the corresponding
  // ix arrays in the two compartments
  VoxelIndexer Aindexer(A->getImageSize(), A->getVoxels());
  VoxelIndexer Bindexer(B->getImageSize(), B->getVoxels());
  for (const auto &[pA, pB] : *voxelPairs) {
    // get the flux direction between the pair of voxels
    auto fluxDirection = [](const Voxel &a, const Voxel &b) {
      auto dir{a - b};
      if (dir.z != 0) {
        return FLUX_DIRECTION::Z;
      } else if (dir.p.y() != 0) {
        return FLUX_DIRECTION::Y;
      }
      return FLUX_DIRECTION::X;
    }(pA, pB);
    auto iA{Aindexer.getIndex(pA)};
    auto iB{Bindexer.getIndex(pB)};
    indexPairs[fluxDirection].emplace_back(iA.value(), iB.value());
  }
  images.fill(0);
  for (const auto &[vA, vB] : *voxelPairs) {
    images[vA.z].setPixel(vA.p, colA);
    images[vB.z].setPixel(vB.p, colB);
  }
}

const std::string &Membrane::getId() const { return id; }

void Membrane::setId(const std::string &membraneId) { id = membraneId; }

const Compartment *Membrane::getCompartmentA() const { return compA; }

const Compartment *Membrane::getCompartmentB() const { return compB; }

const std::vector<std::pair<std::size_t, std::size_t>> &
Membrane::getIndexPairs(FLUX_DIRECTION fluxDirection) const {
  return indexPairs[fluxDirection];
}

const common::ImageStack &Membrane::getImages() const { return images; }

Field::Field(const Compartment *compartment, std::string specID,
             double diffConst, QRgb col)
    : id(std::move(specID)), comp(compartment), diffusionConstant(diffConst),
      colour(col), conc(compartment->nVoxels(), 0.0) {
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
  SPDLOG_INFO("  - importing from sbml array of volume {}",
              sbmlConcentrationArray.size());
  const auto &imageSize{comp->getImageSize()};
  int nx{imageSize.width()};
  int ny{imageSize.height()};
  int nz{static_cast<int>(imageSize.depth())};
  if (sbmlConcentrationArray.size() != imageSize.nVoxels()) {
    SPDLOG_ERROR("  - mismatch between array size [{}]"
                 " and compartment image size [{}x{}x{} = {}]",
                 sbmlConcentrationArray.size(), nx, ny, nz,
                 imageSize.nVoxels());
    throw std::invalid_argument("invalid array size");
  }
  // NOTE: order of concentration array is
  // [ (x=0,y=0,z=0), (x=1,y=0,z=0), ... (x=0,y=1,z=0), ... ]
  // NOTE: y=0 is at the bottom, QImage has y=0 at the top, so flip y-coord
  for (std::size_t i = 0; i < comp->nVoxels(); ++i) {
    const auto &v{comp->getVoxel(i)};
    int arrayIndex{v.p.x() + nx * (ny - 1 - v.p.y()) +
                   nx * ny * static_cast<int>(v.z)};
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
  std::ranges::fill(conc, concentration);
  isUniformConcentration = true;
}

common::ImageStack Field::getConcentrationImages() const {
  common::ImageStack images{comp->getImageSize(),
                            QImage::Format_ARGB32_Premultiplied};
  images.fill(0);
  // for now rescale conc to [0,1] to multiply species colour
  double cmax{common::max(conc)};
  constexpr double concMinNonZeroThreshold{1e-15};
  if (cmax < concMinNonZeroThreshold) {
    cmax = 1.0;
  }
  for (std::size_t i = 0; i < comp->nVoxels(); ++i) {
    double scale = conc[i] / cmax;
    const auto &v{comp->getVoxel(i)};
    int r{static_cast<int>(scale * qRed(colour))};
    int g{static_cast<int>(scale * qGreen(colour))};
    int b{static_cast<int>(scale * qBlue(colour))};
    images[v.z].setPixel(v.p, qRgb(r, g, b));
  }
  return images;
}

std::vector<double>
Field::getConcentrationImageArray(bool maskAndInvertY) const {
  std::vector<double> a;
  const auto &imageSize{comp->getImageSize()};
  int nx{imageSize.width()};
  int ny{imageSize.height()};
  if (maskAndInvertY) {
    // y=0 at top of image & set voxels outside of compartment to zero
    a.resize(imageSize.nVoxels(), 0.0);
    for (std::size_t i = 0; i < comp->nVoxels(); ++i) {
      auto v{comp->getVoxel(i)};
      a[static_cast<std::size_t>(v.p.x() + nx * v.p.y()) +
        (static_cast<std::size_t>(nx * ny) * v.z)] = conc[i];
    }
  } else {
    // y=0 at bottom, set voxels outside of compartment to nearest valid voxel
    a.reserve(imageSize.nVoxels());
    for (std::size_t i : comp->getArrayPoints()) {
      a.push_back(conc[i]);
    }
  }
  return a;
}

void Field::setCompartment(const Compartment *compartment) {
  SPDLOG_DEBUG("Changing compartment to {}", compartment->getId());
  comp = compartment;
  conc.assign(compartment->nVoxels(), 0.0);
}

} // namespace sme::geometry
