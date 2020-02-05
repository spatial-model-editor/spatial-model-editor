#include "dunesim.hpp"

#include "duneini.hpp"

#ifdef __GNUC__
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wsign-conversion"
#pragma GCC diagnostic ignored "-Wold-style-cast"
#pragma GCC diagnostic ignored "-Wconversion"
#pragma GCC diagnostic ignored "-Wsign-conversion"
#pragma GCC diagnostic ignored "-Wfloat-conversion"
#pragma GCC diagnostic ignored "-Wunused-parameter"
#pragma GCC diagnostic ignored "-Wshadow"
#pragma GCC diagnostic ignored "-Wpedantic"
#pragma GCC diagnostic ignored "-Wformat-nonliteral"
#pragma GCC diagnostic ignored "-Wnon-virtual-dtor"
#pragma GCC diagnostic ignored "-Wunused-local-typedefs"
#pragma GCC diagnostic ignored "-Woverloaded-virtual"
#pragma GCC diagnostic ignored "-Wpessimizing-move"
#pragma GCC diagnostic ignored "-Wdouble-promotion"
#if (__GNUC__ > 6)
#pragma GCC diagnostic ignored "-Wsubobject-linkage"
#endif
#if (__GNUC__ > 8)
#pragma GCC diagnostic ignored "-Wdeprecated-copy"
#endif
#endif

#include <dune_config.h>

#include <dune/common/exceptions.hh>
#include <dune/common/parallel/mpihelper.hh>
#include <dune/common/parametertree.hh>
#include <dune/common/parametertreeparser.hh>
#include <dune/copasi/common/enum.hh>
#include <dune/copasi/grid/mark_stripes.hh>
#include <dune/copasi/grid/multidomain_gmsh_reader.hh>
#include <dune/copasi/model/base.hh>
#include <dune/copasi/model/diffusion_reaction.hh>
#include <dune/copasi/model/multidomain_diffusion_reaction.hh>
#include <dune/grid/multidomaingrid.hh>
#include <dune/grid/uggrid.hh>
#include <dune/logging/logging.hh>
#include <dune/logging/loggingstream.hh>

#ifdef __GNUC__
#pragma GCC diagnostic pop
#endif

#include <QFile>
#include <QImage>
#include <QPainter>
#include <algorithm>

#include "logger.hpp"
#include "mesh.hpp"
#include "pde.hpp"
#include "sbml.hpp"
#include "symbolic.hpp"
#include "tiff.hpp"
#include "utils.hpp"

using QTriangleF = std::array<QPointF, 3>;

constexpr int DuneDimensions = 2;
constexpr int DuneFEMOrder = 1;

using HostGrid = Dune::UGGrid<DuneDimensions>;
using MDGTraits = Dune::mdgrid::DynamicSubDomainCountTraits<DuneDimensions, 1>;
using Grid = Dune::mdgrid::MultiDomainGrid<HostGrid, MDGTraits>;
using ModelTraits =
    Dune::Copasi::ModelMultiDomainP0PkDiffusionReactionTraits<Grid,
                                                              DuneFEMOrder>;
using Model = Dune::Copasi::ModelMultiDomainDiffusionReaction<ModelTraits>;

namespace sim {

class DuneSim::DuneImpl {
 public:
  Dune::ParameterTree config;
  std::shared_ptr<Grid> grid_ptr;
  std::shared_ptr<HostGrid> host_grid_ptr;
  std::unique_ptr<Model> model;
  void init(const std::string &iniFile);
};

void DuneSim::DuneImpl::init(const std::string &iniFile) {
  std::stringstream ssIni(iniFile);
  Dune::ParameterTreeParser::readINITree(ssIni, config);

  // init Dune logging if not already done
  if (!Dune::Logging::Logging::initialized()) {
    Dune::Logging::Logging::init(
        Dune::FakeMPIHelper::getCollectiveCommunication(),
        config.sub("logging"));
    if (SPDLOG_ACTIVE_LEVEL >= 2) {
      // for release builds disable DUNE logging
      Dune::Logging::Logging::mute();
    }
  }
  // NB: msh file needs to be file for gmshreader
  // todo: generate DUNE mesh directly
  std::tie(grid_ptr, host_grid_ptr) =
      Dune::Copasi::MultiDomainGmshReader<Grid>::read("grid.msh", config);

  // initialize model
  model = std::make_unique<Model>(grid_ptr, config.sub("model"));
}

void DuneSim::initCompartmentNames() {
  compartmentSpeciesIndex.clear();
  std::size_t compIndex = 0;
  for (const auto &name :
       pDuneImpl->config.sub("model.compartments").getValueKeys()) {
    auto duneCompIndex =
        pDuneImpl->config.sub("model.compartments").get<int>(name);
    SPDLOG_DEBUG("compartment: {} - Dune index {}", name, duneCompIndex);
    const auto &gv =
        pDuneImpl->grid_ptr->subDomain(duneCompIndex).leafGridView();
    if (std::all_of(elements(gv).begin(), elements(gv).end(),
                    [](const auto &e) { return e.type().isTriangle(); })) {
      if (static_cast<std::size_t>(duneCompIndex) != compIndex) {
        SPDLOG_ERROR(
            "Dune compartment indices must match order: comp {} has DUNE index "
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
                                                double scale) {
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
  return std::make_pair<QPoint, QPoint>(
      QPoint(static_cast<int>(fmin.x() / scale),
             static_cast<int>(fmin.y() / scale)),
      QPoint(static_cast<int>(fmax.x() / scale),
             static_cast<int>(fmax.y() / scale)));
}

static std::size_t getIxValidNeighbour(std::size_t ix,
                                       const std::vector<bool> &ixValid,
                                       const geometry::Compartment *g) {
  std::vector<std::size_t> queue{ix};
  std::size_t queueIndex = 0;
  // return nearest neighbour if valid, otherwise add to queue
  for (std::size_t iter = 0; iter < 1024 * ixValid.size(); ++iter) {
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
  SPDLOG_ERROR("Failed to find valid neighbour of pixel {}", ix);
  return 0;
}

void DuneSim::updatePixels() {
  pixels.clear();
  missingPixels.clear();
  for (std::size_t compIndex = 0; compIndex < compartmentSpeciesIndex.size();
       ++compIndex) {
    auto &pixelsComp = pixels.emplace_back();
    const auto &gridview =
        pDuneImpl->grid_ptr->subDomain(static_cast<int>(compIndex))
            .leafGridView();
    const auto &compName = compartmentDuneNames[compIndex];
    SPDLOG_TRACE("compartment[{}]: {}", compIndex, compName);
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
      auto [pMin, pMax] = getBoundingBox({{c0, c1, c2}}, pixelSize);
      // note: mesh points constructed at pixel points (i.e. bottom left
      // corner of pixel) so here we also evaluate at this point

      // todo: consider having mesh points in centre of pixel, and then also
      // evaluate the dune grid functions in the centre
      SPDLOG_TRACE("  - bounding box ({},{}) - ({},{})", pMin.x(), pMin.y(),
                   pMax.x(), pMax.y());
      for (int x = pMin.x(); x < pMax.x() + 1; ++x) {
        for (int y = pMin.y(); y < pMax.y() + 1; ++y) {
          auto localPoint =
              e.geometry().local({static_cast<double>(x) * pixelSize,
                                  static_cast<double>(y) * pixelSize});
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

DuneSim::DuneSim(const sbml::SbmlDocWrapper &sbmlDoc)
    : pDuneImpl(std::make_unique<DuneImpl>()),
      geometryImageSize(sbmlDoc.getCompartmentImage().size()),
      pixelSize(sbmlDoc.getPixelWidth()) {
  dune::DuneConverter dc(sbmlDoc, 1e-6);
  // export gmsh file `grid.msh` in the same dir
  QFile f2("grid.msh");
  if (f2.open(QIODevice::WriteOnly | QIODevice::Text)) {
    f2.write(sbmlDoc.mesh->getGMSH(dc.getGMSHCompIndices()).toUtf8());
    f2.close();
  } else {
    SPDLOG_ERROR("Cannot write to file grid.msh");
  }

  pDuneImpl->init(dc.getIniFile().toStdString());

  initCompartmentNames();
  initSpeciesIndices();
  for (const auto &compId : sbmlDoc.compartments) {
    const auto &comp = sbmlDoc.mapCompIdToGeometry.at(compId);
    compartmentPointIndex.emplace_back(comp.getCompartmentImage().size(),
                                       comp.getPixels());
    compartmentGeometry.push_back(&comp);
    auto nPixels = comp.getPixels().size();
    // todo: don't allocate wasted space for constant species here
    auto nSpecies = static_cast<std::size_t>(sbmlDoc.species.at(compId).size());
    concentration.emplace_back(nPixels * nSpecies, 0.0);
  }
  updatePixels();
  updateSpeciesConcentrations();
}

DuneSim::~DuneSim() = default;

void DuneSim::doTimestep(double t, double dt) {
  pDuneImpl->model->suggest_timestep(dt);
  pDuneImpl->model->end_time() = pDuneImpl->model->current_time() + t;
  pDuneImpl->model->run();
  updateSpeciesConcentrations();
}

const std::vector<double> &DuneSim::getConcentrations(
    std::size_t compartmentIndex) const {
  return concentration[compartmentIndex];
}

void DuneSim::updateSpeciesConcentrations() {
  using GF = std::remove_reference_t<decltype(
      *pDuneImpl->model->get_grid_function(pDuneImpl->model->states(), 0, 0)
           .get())>;
  for (std::size_t iComp = 0; iComp < compartmentSpeciesIndex.size(); ++iComp) {
    std::size_t nSpecies = compartmentSpeciesIndex[iComp].size();
    // get grid function for each species in this compartment
    std::vector<std::shared_ptr<GF>> gridFunctions;
    gridFunctions.reserve(nSpecies);
    for (std::size_t iSpecies = 0; iSpecies < nSpecies; ++iSpecies) {
      gridFunctions.push_back(pDuneImpl->model->get_grid_function(
          pDuneImpl->model->states(), iComp, iSpecies));
    }
    const auto &gridview =
        pDuneImpl->grid_ptr->subDomain(static_cast<int>(iComp)).leafGridView();
    std::size_t iTriangle = 0;
    for (const auto e : elements(gridview)) {
      SPDLOG_TRACE("triangle {}", iTriangle);
      for (const auto &[ix, point] : pixels[iComp][iTriangle]) {
        // evaluate DUNE grid function at this pixel location
        // convert pixel->global->local
        Dune::FieldVector<double, 2> localPoint = {point[0], point[1]};
        SPDLOG_TRACE("  - pixel ({}) -> -> local ({},{})", ix, localPoint[0],
                     localPoint[1]);
        GF::Traits::RangeType result;
        std::vector<double> localConcs;
        localConcs.reserve(nSpecies);
        for (std::size_t iSpecies = 0; iSpecies < nSpecies; ++iSpecies) {
          std::size_t externalSpeciesIndex =
              compartmentSpeciesIndex[iComp][iSpecies];
          gridFunctions[iSpecies]->evaluate(e, localPoint, result);
          SPDLOG_TRACE("    - species[{}] = {}", iSpecies, result[0]);
          // replace negative values with zero
          concentration[iComp][ix * nSpecies + externalSpeciesIndex] =
              result[0] < 0 ? 0 : result[0];
        }
      }
      ++iTriangle;
    }
    // fill in missing pixels with neighbouring value
    for (const auto &[ixMissing, ixNeighbour] : missingPixels[iComp]) {
      for (std::size_t iSpecies = 0; iSpecies < nSpecies; ++iSpecies) {
        concentration[iComp][ixMissing * nSpecies + iSpecies] =
            concentration[iComp][ixNeighbour * nSpecies + iSpecies];
      }
    }
  }
}

}  // namespace sim
