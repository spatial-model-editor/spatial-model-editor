#include "dunesim.hpp"
#include "dune_headers.hpp"
#include "duneconverter.hpp"
#include "dunefunction.hpp"
#include "dunegrid.hpp"
#include "dunesim_impl.hpp"
#include "dunesim_impl_coupled.hpp"
#include "dunesim_impl_independent.hpp"
#include "geometry.hpp"
#include "logger.hpp"
#include "model.hpp"
#include "model_compartments.hpp"
#include "model_geometry.hpp"
#include "utils.hpp"
#include <QFile>
#include <QImage>
#include <QPainter>
#include <algorithm>
#include <numeric>

using QTriangleF = std::array<QPointF, 3>;

namespace simulate {

void DuneSim::initCompartmentNames() {
  compartmentSpeciesIndex.clear();
  std::size_t compIndex = 0;
  for (const auto &name :
       pDuneImpl->config.sub("model.compartments").getValueKeys()) {
    auto duneCompIndex =
        pDuneImpl->config.sub("model.compartments").get<int>(name);
    SPDLOG_DEBUG("compartment: {} - Dune index {}", name, duneCompIndex);
    const auto &gv = pDuneImpl->grid->subDomain(duneCompIndex).leafGridView();
    if (std::all_of(elements(gv).begin(), elements(gv).end(),
                    [](const auto &e) { return e.type().isTriangle(); })) {
      if (static_cast<std::size_t>(duneCompIndex) != compIndex) {
        SPDLOG_ERROR(
            "Dune compartment indices must match order: comp {} has DUNE "
            "index "
            "{}",
            compIndex, duneCompIndex);
      }
      SPDLOG_DEBUG("  -> compartment of triangles: adding");
      compartmentSpeciesIndex.emplace_back();
      compartmentDuneNames.push_back(name);
      ++compIndex;
    } else {
      SPDLOG_DEBUG("  -> not a compartment of triangles: ignoring");
    }
  }
}

void DuneSim::initSpeciesIndices() {
  std::size_t nComps = compartmentSpeciesIndex.size();
  for (std::size_t iComp = 0; iComp < nComps; ++iComp) {
    const auto &compName = compartmentDuneNames[iComp];
    SPDLOG_DEBUG("compartment[{}]: {}", iComp, compName);
    auto duneNames =
        pDuneImpl->config.sub("model." + compName + ".initial").getValueKeys();
    // create {0, 1, 2, ...} initial species indices
    auto &indices = compartmentSpeciesIndex[iComp];
    indices.resize(duneNames.size());
    std::iota(indices.begin(), indices.end(), 0);
    // sort these indices by duneNames, i.e. find indices that would result in
    // a sorted duneNames
    std::sort(indices.begin(), indices.end(),
              [&n = duneNames](std::size_t i1, std::size_t i2) {
                return n[i1] < n[i2];
              });
    // indices[i] is now the Dune index of species i
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
  pixels.clear();
  missingPixels.clear();
  SPDLOG_TRACE("pixel size: {}", pixelSize);
  for (std::size_t compIndex = 0; compIndex < compartmentSpeciesIndex.size();
       ++compIndex) {
    auto &pixelsComp = pixels.emplace_back();
    const auto &gridview =
        pDuneImpl->grid->subDomain(static_cast<int>(compIndex)).leafGridView();
    SPDLOG_TRACE("compartment[{}]: {}", compIndex,
                 compartmentDuneNames[compIndex]);
    const auto &qpi = compartmentPointIndex[compIndex];
    std::vector<bool> ixAssigned(qpi.getNumPoints(), false);
    // get local coord for each pixel in each triangle
    for (const auto e : elements(gridview)) {
      auto &pixelsTriangle = pixelsComp.emplace_back();
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
              {static_cast<double>(x) * pixelSize + pixelOrigin.x(),
               static_cast<double>(y) * pixelSize + pixelOrigin.y()});
          // note: qpi/QImage has (0,0) in top-left corner:
          QPoint pix = QPoint(x, geometryImageSize.height() - 1 - y);
          if (auto ix = qpi.getIndex(pix);
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
    const auto *geom = compartmentGeometry[compIndex];
    auto &missing = missingPixels.emplace_back();
    for (std::size_t ix = 0; ix < ixAssigned.size(); ++ix) {
      if (!ixAssigned[ix]) {
        SPDLOG_TRACE("pixel {} not in a triangle", ix);
        // find a neigbouring valid pixel
        auto ixNeighbour = getIxValidNeighbour(ix, ixAssigned, geom);
        SPDLOG_TRACE("  -> using concentration from pixel {}", ixNeighbour);
        missing.push_back({ix, ixNeighbour});
      }
    }
  }
}

DuneSim::DuneSim(
    const model::Model &sbmlDoc, const std::vector<std::string> &compartmentIds,
    const std::vector<std::vector<std::string>> &compartmentSpeciesIds,
    std::size_t order)
    : geometryImageSize{sbmlDoc.getGeometry().getImage().size()},
      pixelSize{sbmlDoc.getGeometry().getPixelWidth()},
      pixelOrigin{sbmlDoc.getGeometry().getPhysicalOrigin()}, integratorOrder{
                                                                  order} {
  simulate::DuneConverter dc(sbmlDoc, false, 1e-6);
  if (integratorOrder != 1) {
    // for now we only support 1st order FEM
    // in future could add:
    //  - 0th order a.k.a. FVM for independent compartment models
    //  - 2nd order FEM for both types of models
    SPDLOG_WARN("Invalid order {} requested - using 1st order FEM instead",
                integratorOrder);
    integratorOrder = 1;
  }
  try {
    if (dc.hasIndependentCompartments()) {
      pDuneImpl = std::make_unique<DuneImplIndependent<1>>(dc);
    } else {
      pDuneImpl = std::make_unique<DuneImplCoupled<1>>(dc);
    }
    pDuneImpl->setInitial(dc);
    initCompartmentNames();
    initSpeciesIndices();

    for (std::size_t compIndex = 0; compIndex < compartmentIds.size();
         ++compIndex) {
      const auto &compId = compartmentIds[compIndex];
      SPDLOG_INFO("compartmentId: {}", compId);
      const auto *comp =
          sbmlDoc.getCompartments().getCompartment(compId.c_str());
      compartmentPointIndex.emplace_back(comp->getCompartmentImage().size(),
                                         comp->getPixels());
      compartmentGeometry.push_back(comp);
      auto nPixels = comp->getPixels().size();
      SPDLOG_INFO("  - {} pixels", nPixels);
      // todo: don't allocate wasted space for constant species here
      auto nSpecies = compartmentSpeciesIds[compIndex].size();
      SPDLOG_INFO("  - {} species", nSpecies);
      concentration.emplace_back(nPixels * nSpecies, 0.0);
    }

    updatePixels();
    updateSpeciesConcentrations();
  } catch (const Dune::Exception &e) {
    currentErrorMessage = e.what();
  }
}

DuneSim::~DuneSim() = default;

void DuneSim::setIntegrationOrder(std::size_t order) {
  if (order != integratorOrder) {
    SPDLOG_WARN(
        "Integration order cannot be changed once DUNE simulation is created - "
        "ignoring request to change order from {} to {}",
        integratorOrder, order);
  }
}

std::size_t DuneSim::getIntegrationOrder() const { return integratorOrder; }

void DuneSim::setIntegratorError(const IntegratorError &err) { errMax = err; }

IntegratorError DuneSim::getIntegratorError() const { return errMax; }

void DuneSim::setMaxDt(double maxDt) {
  SPDLOG_INFO("Setting max timestep: {}", maxDt);
  maxTimestep = maxDt;
}

double DuneSim::getMaxDt() const { return maxTimestep; }

void DuneSim::setMaxThreads([[maybe_unused]] std::size_t maxThreads) {
  SPDLOG_INFO("DUNE is single threaded - ignoring");
}

std::size_t DuneSim::getMaxThreads() const { return 0; }

std::size_t DuneSim::run(double time) {
  if (pDuneImpl == nullptr) {
    return 0;
  }
  try {
    pDuneImpl->run(time, maxTimestep);
    updateSpeciesConcentrations();
    currentErrorMessage.clear();
  } catch (const Dune::SolverAbort &e) {
    currentErrorMessage = e.what();
  } catch (const Dune::PDELab::NewtonLinearSolverError &e) {
    currentErrorMessage = e.what();
  } catch (const Dune::PDELab::NewtonLineSearchError &e) {
    currentErrorMessage = e.what();
  }
  return 0;
}

const std::vector<double> &
DuneSim::getConcentrations(std::size_t compartmentIndex) const {
  return concentration[compartmentIndex];
}

std::string DuneSim::errorMessage() const { return currentErrorMessage; }

void DuneSim::updateSpeciesConcentrations() {
  for (std::size_t iComp = 0; iComp < compartmentSpeciesIndex.size(); ++iComp) {
    std::size_t nSpecies = compartmentSpeciesIndex[iComp].size();
    pDuneImpl->updateGridFunctions(iComp, nSpecies);
    const auto &gridview =
        pDuneImpl->grid->subDomain(static_cast<int>(iComp)).leafGridView();
    std::size_t iTriangle = 0;
    for (const auto e : elements(gridview)) {
      SPDLOG_TRACE("triangle {} ({})", iTriangle,
                   utils::decltypeStr<decltype(e)>());
      for (const auto &[ix, point] : pixels[iComp][iTriangle]) {
        // evaluate DUNE grid function at this pixel location
        // convert pixel->global->local
        Dune::FieldVector<double, 2> localPoint = {point[0], point[1]};
        SPDLOG_TRACE("  - pixel ({}) -> -> local ({},{})", ix, localPoint[0],
                     localPoint[1]);
        std::vector<double> localConcs;
        localConcs.reserve(nSpecies);
        for (std::size_t iSpecies = 0; iSpecies < nSpecies; ++iSpecies) {
          std::size_t externalSpeciesIndex =
              compartmentSpeciesIndex[iComp][iSpecies];
          double result =
              pDuneImpl->evaluateGridFunction(iSpecies, e, localPoint);
          SPDLOG_TRACE("    - species[{}] = {}", iSpecies, result);
          // replace negative values with zero
          concentration[iComp][ix * nSpecies + externalSpeciesIndex] =
              result < 0 ? 0 : result;
        }
      }
      ++iTriangle;
    }
  }
  // fill in missing pixels with neighbouring value
  for (std::size_t iComp = 0; iComp < compartmentSpeciesIndex.size(); ++iComp) {
    std::size_t nSpecies = compartmentSpeciesIndex[iComp].size();
    for (const auto &[ixMissing, ixNeighbour] : missingPixels[iComp]) {
      for (std::size_t iSpecies = 0; iSpecies < nSpecies; ++iSpecies) {
        concentration[iComp][ixMissing * nSpecies + iSpecies] =
            concentration[iComp][ixNeighbour * nSpecies + iSpecies];
      }
    }
  }
}

} // namespace simulate
