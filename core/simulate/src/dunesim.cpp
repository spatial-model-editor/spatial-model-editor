#include "dunesim.hpp"
#include "dune_headers.hpp"
#include "dunefunction.hpp"
#include "dunegrid.hpp"
#include "dunesim_impl.hpp"
#include "dunesim_impl_coupled.hpp"
#include "dunesim_impl_independent.hpp"
#include "sme/duneconverter.hpp"
#include "sme/geometry.hpp"
#include "sme/logger.hpp"
#include "sme/model.hpp"
#include "sme/model_compartments.hpp"
#include "sme/model_geometry.hpp"
#include "sme/utils.hpp"
#include <QElapsedTimer>
#include <QFile>
#include <QImage>
#include <QPainter>
#include <algorithm>
#include <numeric>

using QTriangleF = std::array<QPointF, 3>;

namespace sme::simulate {

void DuneSim::initDuneSimCompartments(
    const std::vector<const geometry::Compartment *> &comps) {
  duneCompartments.clear();
  auto compartmentNames{
      pDuneImpl->configs[0].sub("model.compartments").getValueKeys()};
  for (std::size_t i = 1; i < pDuneImpl->configs.size(); ++i) {
    // if we have multiple configs each has a single compartment
    compartmentNames.push_back(
        pDuneImpl->configs[i].sub("model.compartments").getValueKeys().front());
  }
  std::size_t compIndex{0};
  for (const auto &compartmentName : compartmentNames) {
    SPDLOG_DEBUG("compartment: {} - Dune index {}", compartmentName, compIndex);
    if (auto iter{std::find_if(comps.cbegin(), comps.cend(),
                               [&compartmentName](const auto *c) {
                                 return c->getId() == compartmentName;
                               })};
        iter != comps.cend()) {
      const auto *comp{*iter};
      std::size_t iModel{compIndex};
      if (pDuneImpl->configs.size() == 1) {
        iModel = 0;
      }
      auto speciesNames{pDuneImpl->configs[iModel]
                            .sub("model." + compartmentName + ".initial")
                            .getValueKeys()};
      auto duneToSmeSpeciesIndices{
          common::getIndicesOfSortedVector(speciesNames)};

      for (std::size_t i = 0; i < speciesNames.size(); ++i) {
        SPDLOG_INFO("  - DUNE species {} -> SME species {} [{}]", i,
                    duneToSmeSpeciesIndices[i],
                    speciesNames[duneToSmeSpeciesIndices[i]]);
      }
      // duneToSmeSpeciesIndices[i] is now the SME index of the species with
      // Dune index i
      auto imgVolume{comp->getCompartmentImages().volume()};
      auto nPixels{comp->getVoxels().size()};
      SPDLOG_INFO("  - {} pixels", nPixels);
      // todo: don't allocate wasted space for constant species here
      auto nSpecies{speciesNames.size()};
      SPDLOG_INFO("  - {} species", nSpecies);
      duneCompartments.push_back(
          {compartmentName,
           compIndex,
           duneToSmeSpeciesIndices,
           geometry::VoxelIndexer(imgVolume, comp->getVoxels()),
           comp,
           {},
           {},
           std::vector<double>(nPixels * nSpecies, 0.0)});
    } else {
      SPDLOG_DEBUG(
          "  -> empty compartment (apart from dummy species): ignoring");
    }
    ++compIndex;
  }
}

static std::pair<QPoint, QPoint> getBoundingBox(const QTriangleF &t,
                                                double pixelSize,
                                                const QPointF &pixelOrigin) {
  // get triangle bounding box in physical units
  QPointF fmin(t[0].x(), t[0].y());
  QPointF fmax = fmin;
  for (std::size_t i = 1; i < 3; ++i) {
    fmin.setX(std::min(fmin.x(), t[i].x()));
    fmax.setX(std::max(fmax.x(), t[i].x()));
    fmin.setY(std::min(fmin.y(), t[i].y()));
    fmax.setY(std::max(fmax.y(), t[i].y()));
  }
  // convert physical points to pixel locations
  fmin -= pixelOrigin;
  fmin /= pixelSize;
  fmax -= pixelOrigin;
  fmax /= pixelSize;
  return std::make_pair<QPoint, QPoint>(
      QPoint(static_cast<int>(fmin.x()), static_cast<int>(fmin.y())),
      QPoint(static_cast<int>(fmax.x()), static_cast<int>(fmax.y())));
}

static std::size_t getIxValidNeighbour(std::size_t ix,
                                       const std::vector<bool> &ixValid,
                                       const geometry::Compartment *g) {
  std::vector<std::size_t> queue{ix};
  std::size_t queueIndex = 0;
  // return nearest neighbour if valid, otherwise add to queue
  for (std::size_t iter = 0; iter < 10 * ixValid.size(); ++iter) {
    std::size_t i = queue[queueIndex];
    for (auto iy : {g->up_x(i), g->dn_x(i), g->up_y(i), g->dn_y(i)}) {
      if (ixValid[iy]) {
        return iy;
      } else {
        queue.push_back(iy);
      }
    }
    ++queueIndex;
  }
  SPDLOG_WARN("Failed to find valid neighbour of pixel {}", ix);
  return 0;
}

void DuneSim::updatePixels() {
  SPDLOG_TRACE("pixel size: {}", pixelSize);
  for (auto &comp : duneCompartments) {
    comp.pixels.clear();
    comp.missingPixels.clear();
    const auto &gridview{
        pDuneImpl->grid->subDomain(static_cast<int>(comp.index))
            .leafGridView()};
    SPDLOG_TRACE("compartment[{}]: {}", comp.index, comp.name);
    const auto &qpi{comp.voxelIndexer};
    std::vector<bool> ixAssigned(qpi.size(), false);
    // get local coord for each pixel in each triangle
    for (const auto e : elements(gridview)) {
      auto &pixelsTriangle = comp.pixels.emplace_back();
      const auto &geo = e.geometry();
      assert(geo.type().isTriangle());
      auto ref = Dune::referenceElement(geo);
      QPointF c0(geo.corner(0)[0], geo.corner(0)[1]);
      QPointF c1(geo.corner(1)[0], geo.corner(1)[1]);
      QPointF c2(geo.corner(2)[0], geo.corner(2)[1]);
      auto [pMin, pMax] =
          getBoundingBox({{c0, c1, c2}}, pixelSize, pixelOrigin);
      SPDLOG_TRACE("  - bounding box ({},{}) - ({},{})", pMin.x(), pMin.y(),
                   pMax.x(), pMax.y());
      for (int x = pMin.x(); x < pMax.x() + 1; ++x) {
        for (int y = pMin.y(); y < pMax.y() + 1; ++y) {
          auto localPoint = e.geometry().local(
              {(static_cast<double>(x) + 0.5) * pixelSize + pixelOrigin.x(),
               (static_cast<double>(y) + 0.5) * pixelSize + pixelOrigin.y()});
          // note: qpi/QImage has (0,0) in top-left corner:
          common::Voxel vox{x, geometryImageSize.height() - 1 - y, 0};
          if (auto ix{qpi.getIndex(vox)};
              ix.has_value() && ref.checkInside(localPoint)) {
            pixelsTriangle.push_back({*ix, {localPoint[0], localPoint[1]}});
            ixAssigned[*ix] = true;
          }
        }
      }
      SPDLOG_TRACE("    - found {} pixels", pixelsTriangle.size());
    }
    // Deal with pixels that fell outside of mesh (either in a membrane, or
    // where the mesh boundary differs a little from the pixel boundary).
    // For now we just set the value to the nearest pixel from the same
    // compartment which does lie inside a triangle
    for (std::size_t ix = 0; ix < ixAssigned.size(); ++ix) {
      if (!ixAssigned[ix]) {
        SPDLOG_DEBUG("pixel {} not in a triangle", ix);
        // find a neighbouring valid pixel
        auto ixNeighbour = getIxValidNeighbour(ix, ixAssigned, comp.geometry);
        SPDLOG_DEBUG("  -> using concentration from pixel {}", ixNeighbour);
        comp.missingPixels.push_back({ix, ixNeighbour});
      }
    }
  }
}

DuneSim::DuneSim(
    const model::Model &sbmlDoc, const std::vector<std::string> &compartmentIds,
    const std::map<std::string, double, std::less<>> &substitutions)
    : geometryImageSize{sbmlDoc.getGeometry().getImages()[0].size()},
      pixelSize{sbmlDoc.getGeometry().getVoxelSize().width()},
      pixelOrigin{sbmlDoc.getGeometry().getPhysicalOrigin().p} {
  try {
    const auto &lengthUnit{sbmlDoc.getUnits().getLength()};
    const auto &volumeUnit{sbmlDoc.getUnits().getVolume()};
    volOverL3 = model::getVolOverL3(lengthUnit, volumeUnit);

    simulate::DuneConverter dc(sbmlDoc, substitutions, false);
    const auto &options{sbmlDoc.getSimulationSettings().options.dune};
    if (options.discretization != DuneDiscretizationType::FEM1) {
      // for now we only support 1st order FEM
      // in future could add:
      //  - 0th order a.k.a. FVM for independent compartment models
      //  - 2nd order FEM for both types of models
      SPDLOG_ERROR("Invalid integrator type requested");
      throw std::runtime_error("Invalid integrator type requested");
    }
    if (dc.getIniFiles().empty()) {
      currentErrorMessage = "Nothing to simulate";
      SPDLOG_WARN("{}", currentErrorMessage);
      return;
    }
    if (dc.hasIndependentCompartments()) {
      pDuneImpl = std::make_unique<DuneImplIndependent<1>>(dc, options);
    } else {
      pDuneImpl = std::make_unique<DuneImplCoupled<1>>(dc, options);
    }
    pDuneImpl->setInitial(dc);
    std::vector<const geometry::Compartment *> comps;
    for (const auto &compartmentId : compartmentIds) {
      comps.push_back(
          sbmlDoc.getCompartments().getCompartment(compartmentId.c_str()));
    }
    initDuneSimCompartments(comps);
    updatePixels();
    updateSpeciesConcentrations();
  } catch (const Dune::Exception &e) {
    currentErrorMessage = e.what();
    SPDLOG_ERROR("{}", currentErrorMessage);
  } catch (const std::runtime_error &e) {
    SPDLOG_ERROR("runtime_error: {}", e.what());
    currentErrorMessage = e.what();
  }
}

DuneSim::~DuneSim() = default;

std::size_t DuneSim::run(double time, double timeout_ms,
                         const std::function<bool()> &stopRunningCallback) {
  if (pDuneImpl == nullptr) {
    return 0;
  }
  QElapsedTimer timer;
  timer.start();
  try {
    pDuneImpl->run(time);
    updateSpeciesConcentrations();
    currentErrorMessage.clear();
  } catch (const Dune::Exception &e) {
    currentErrorMessage = e.what();
    SPDLOG_ERROR("{}", currentErrorMessage);
  }
  if (stopRunningCallback && stopRunningCallback()) {
    SPDLOG_DEBUG("Simulation cancelled: requesting stop");
    currentErrorMessage = "Simulation cancelled";
  }
  if (timeout_ms >= 0.0 && static_cast<double>(timer.elapsed()) >= timeout_ms) {
    SPDLOG_DEBUG("Simulation timeout: requesting stop");
    currentErrorMessage = "Simulation timeout";
  }
  return 1;
}

const std::vector<double> &
DuneSim::getConcentrations(std::size_t compartmentIndex) const {
  return duneCompartments[compartmentIndex].concentration;
}

std::size_t DuneSim::getConcentrationPadding() const { return 0; }

const std::string &DuneSim::errorMessage() const { return currentErrorMessage; }

const common::ImageStack &DuneSim::errorImages() const {
  return currentErrorImages;
}

void DuneSim::setStopRequested([[maybe_unused]] bool stop) {
  SPDLOG_DEBUG("Not implemented - ignoring request");
}

void DuneSim::updateSpeciesConcentrations() {
  for (auto &comp : duneCompartments) {
    const std::size_t nSpecies{comp.speciesIndices.size()};
    pDuneImpl->updateGridFunctions(comp.index, nSpecies);
    const auto &gridview{
        pDuneImpl->grid->subDomain(static_cast<int>(comp.index))
            .leafGridView()};
    std::size_t iTriangle{0};
    for (const auto e : elements(gridview)) {
      SPDLOG_TRACE("triangle {} ({})", iTriangle,
                   common::decltypeStr<decltype(e)>());
      for (const auto &[ix, point] : comp.pixels[iTriangle]) {
        // evaluate DUNE grid function at this pixel location
        // convert pixel->global->local
        Dune::FieldVector<double, 2> localPoint = {point[0], point[1]};
        SPDLOG_TRACE("  - pixel ({}) -> -> local ({},{})", ix, localPoint[0],
                     localPoint[1]);
        std::vector<double> localConcs;
        localConcs.reserve(nSpecies);
        for (std::size_t iSpecies = 0; iSpecies < nSpecies; ++iSpecies) {
          std::size_t externalSpeciesIndex = comp.speciesIndices[iSpecies];
          double result{
              pDuneImpl->evaluateGridFunction(iSpecies, e, localPoint)};
          // convert result from Amount / Length^3 to Amount / Volume
          result *= volOverL3;
          SPDLOG_TRACE("    - species[{}] = {}", iSpecies, result);
          // replace negative values with zero
          comp.concentration[ix * nSpecies + externalSpeciesIndex] =
              result < 0 ? 0 : result;
        }
      }
      ++iTriangle;
    }
    // fill in missing pixels with neighbouring value
    for (const auto &[ixMissing, ixNeighbour] : comp.missingPixels) {
      for (std::size_t iSpecies = 0; iSpecies < nSpecies; ++iSpecies) {
        comp.concentration[ixMissing * nSpecies + iSpecies] =
            comp.concentration[ixNeighbour * nSpecies + iSpecies];
      }
    }
  }
}

} // namespace sme::simulate
