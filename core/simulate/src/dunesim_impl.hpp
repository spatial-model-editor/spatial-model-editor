// Dune Implementation

#pragma once

#include "dune_headers.hpp"
#include "dunefunction.hpp"
#include "dunegrid.hpp"
#include "sme/duneconverter.hpp"
#include "sme/geometry.hpp"
#include "sme/geometry_utils.hpp"
#include "sme/logger.hpp"
#include "sme/model.hpp"
#include "sme/model_compartments.hpp"
#include "sme/model_geometry.hpp"
#include "sme/simulate_options.hpp"
#include "sme/utils.hpp"
#include <memory>

namespace sme::simulate {

template <int DuneDimensions>
using VoxelLocalPair =
    std::pair<std::size_t, Dune::FieldVector<double, DuneDimensions>>;

using VoxelPair = std::pair<sme::common::Voxel, sme::common::Voxel>;

template <std::size_t DuneDimensions>
VoxelPair
getBoundingBox(const std::vector<std::array<double, DuneDimensions>> &vertices,
               const common::VolumeF &pixelSize,
               const common::VoxelF &pixelOrigin) {
  VoxelPair minMaxPair;
  auto &[pMin, pMax] = minMaxPair;
  // get bounding box that encloses all vertices in physical units
  std::array<double, DuneDimensions> vMin = vertices[0];
  std::array<double, DuneDimensions> vMax = vMin;
  for (const auto &vertex : vertices) {
    for (std::size_t iDim = 0; iDim < vertex.size(); ++iDim) {
      vMin[iDim] = std::min(vMin[iDim], vertex[iDim]);
      vMax[iDim] = std::max(vMax[iDim], vertex[iDim]);
    }
  }
  // convert physical vertices to voxel locations
  pMin.p.rx() =
      static_cast<int>((vMin[0] - pixelOrigin.p.x()) / pixelSize.width());
  pMax.p.rx() =
      static_cast<int>((vMax[0] - pixelOrigin.p.x()) / pixelSize.width());
  pMin.p.ry() =
      static_cast<int>((vMin[1] - pixelOrigin.p.y()) / pixelSize.height());
  pMax.p.ry() =
      static_cast<int>((vMax[1] - pixelOrigin.p.y()) / pixelSize.height());
  pMin.z = 0;
  pMax.z = 0;
  if constexpr (DuneDimensions == 3) {
    pMin.z =
        static_cast<std::size_t>((vMin[2] - pixelOrigin.z) / pixelSize.depth());
    pMax.z =
        static_cast<std::size_t>((vMax[2] - pixelOrigin.z) / pixelSize.depth());
  }
  return minMaxPair;
}

static inline std::size_t getIxValidNeighbour(std::size_t ix,
                                              const std::vector<bool> &ixValid,
                                              const geometry::Compartment *g) {
  std::vector<std::size_t> queue{ix};
  std::size_t queueIndex = 0;
  // return nearest neighbour if valid, otherwise add to queue
  for (std::size_t iter = 0; iter < 10 * ixValid.size(); ++iter) {
    std::size_t i = queue[queueIndex];
    for (auto iy : {g->up_x(i), g->dn_x(i), g->up_y(i), g->dn_y(i), g->up_z(i),
                    g->dn_z(i)}) {
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

template <int DuneDimensions> class DuneImpl {
public:
  explicit DuneImpl(const DuneConverter &dc, const DuneOptions &options,
                    const model::Model &sbmlDoc,
                    const std::vector<std::string> &compartmentIds)
      : speciesNames{dc.getSpeciesNames()},
        geometryImageSize{sbmlDoc.getGeometry().getImages().volume()},
        pixelSize(sbmlDoc.getGeometry().getVoxelSize()),
        pixelOrigin{sbmlDoc.getGeometry().getPhysicalOrigin()} {
    const auto &lengthUnit{sbmlDoc.getUnits().getLength()};
    const auto &volumeUnit{sbmlDoc.getUnits().getVolume()};
    volOverL3 = model::getVolOverL3(lengthUnit, volumeUnit);
    if (SPDLOG_ACTIVE_LEVEL <= SPDLOG_LEVEL_DEBUG) {
      // for debug GUI builds enable verbose DUNE logging
      spdlog::set_level(spdlog::level::trace);
    } else {
      // for release GUI builds disable DUNE logging
      spdlog::set_level(spdlog::level::off);
    }
    std::stringstream ssIni(dc.getIniFile().toStdString());
    Dune::ParameterTreeParser::readINITree(ssIni, config);
    if constexpr (DuneDimensions == 2) {
      std::tie(grid, hostGrid) =
          makeDuneGrid<HostGrid, MDGTraits>(*dc.getMesh());
    } else {
      std::tie(grid, hostGrid) =
          makeDuneGrid<HostGrid, MDGTraits>(*dc.getMesh3d());
    }
    if (options.writeVTKfiles) {
      vtkFilename = "vtk";
    }
    SPDLOG_INFO("parser");
    auto parser_context = std::make_shared<Dune::Copasi::ParserContext>(
        config.sub("parser_context"));
    auto parser_type = Dune::Copasi::string2parser.at(
        config.get("model.parser_type", Dune::Copasi::default_parser_str));
    auto functor_factory =
        std::make_shared<Dune::Copasi::FunctorFactoryParser<DuneDimensions>>(
            parser_type, std::move(parser_context));
    SPDLOG_INFO("model");
    model =
        Dune::Copasi::make_model<Model>(config.sub("model"), functor_factory);
    dt = options.dt;
    SPDLOG_INFO("state");
    state = model->make_state(grid, config.sub("model"));
    SPDLOG_INFO("stepper");
    stepper = std::make_unique<Dune::Copasi::SimpleAdaptiveStepper<
        typename Model::State, double, double>>(
        options.decrease, options.increase, options.minDt, options.maxDt);
    SPDLOG_INFO("step_operator");
    step_operator = model->make_step_operator(*state, config.sub("model"));
    SPDLOG_INFO("done");
    setInitial(dc);
    std::vector<const geometry::Compartment *> comps;
    for (const auto &compartmentId : compartmentIds) {
      comps.push_back(
          sbmlDoc.getCompartments().getCompartment(compartmentId.c_str()));
    }
    initDuneSimCompartments(sbmlDoc.getCompartments().getCompartments());
    updateVoxels();
    updateSpeciesConcentrations();
  }

  ~DuneImpl() = default;

  void run(double time) {
    stepper->evolve(*step_operator, *state, *state, dt, t0 + time).or_throw();
    if (!vtkFilename.empty()) {
      model->write_vtk(*state, vtkFilename, true);
    }
    t0 += time;
    updateSpeciesConcentrations();
  }

  const std::vector<double> &
  getConcentrations(std::size_t compartmentIndex) const {
    return duneCompartments[compartmentIndex].concentration;
  }

private:
  struct DuneSimCompartment {
    std::string name;
    std::size_t index;
    std::size_t nSpecies;
    std::vector<std::string> speciesNames;
    geometry::VoxelIndexer voxelIndexer;
    const geometry::Compartment *geometry;
    // voxel+dune local coords for each triangle
    std::vector<std::vector<VoxelLocalPair<DuneDimensions>>> voxels;
    // index of nearest valid voxel for any missing pixels
    std::vector<std::pair<std::size_t, std::size_t>> missingVoxels;
    std::vector<double> concentration;
  };

  using HostGrid = Dune::UGGrid<DuneDimensions>;
  std::shared_ptr<HostGrid> hostGrid;
  using MDGTraits =
      Dune::mdgrid::DynamicSubDomainCountTraits<DuneDimensions, 10>;
  using Grid = Dune::mdgrid::MultiDomainGrid<HostGrid, MDGTraits>;
  using GridView = typename Grid::LeafGridView;
  using SubGrid = typename Grid::SubDomainGrid;
  using SubGridView = typename SubGrid::LeafGridView;
  using Elem = decltype(*(elements(std::declval<SubGridView>()).begin()));
  using Model = Dune::Copasi::Model<
      Grid, typename Grid::SubDomainGrid::Traits::LeafGridView, double, double>;
  Dune::ParameterTree config;
  std::shared_ptr<Grid> grid;
  std::unordered_map<std::string, std::vector<std::string>> speciesNames;
  std::shared_ptr<Model> model;
  std::shared_ptr<typename Model::State> state;
  std::unordered_map<std::string, typename Model::GridFunction> gridFunctions;
  std::unique_ptr<Dune::Copasi::SimpleAdaptiveStepper<typename Model::State,
                                                      double, double>>
      stepper;
  std::unique_ptr<Dune::PDELab::OneStep<typename Model::State>> step_operator;
  // dimensions of model
  common::Volume geometryImageSize;
  common::VolumeF pixelSize;
  common::VoxelF pixelOrigin;
  double volOverL3;
  double t0{0.0};
  double dt{1e-3};
  std::string vtkFilename{};
  std::vector<DuneSimCompartment> duneCompartments;

  void updateSpeciesConcentrations() {
    for (auto &comp : duneCompartments) {
      SPDLOG_TRACE("compartment {} [{}]", comp.name, comp.index);
      const auto &gridview{
          grid->subDomain(static_cast<int>(comp.index)).leafGridView()};
      std::size_t iSpecies{0};
      for (const auto &speciesName : comp.speciesNames) {
        if (!speciesName.empty()) {
          SPDLOG_TRACE("    - species[{}] '{}'", iSpecies, speciesName);
          auto gridFunc = model->make_compartment_function(state, speciesName);
          auto localGridFunc = localFunction(gridFunc);
          std::size_t iElement{0};
          for (const auto e : elements(gridview)) {
            SPDLOG_TRACE("element {} ({})", iElement,
                         common::decltypeStr<decltype(e)>());
            localGridFunc.bind(e);
            for (const auto &[ix, localPoint] : comp.voxels[iElement]) {
              // evaluate DUNE grid function at this voxel location
              // and convert result from Amount / Length^3 to Amount / Volume
              double result = localGridFunc(localPoint) * volOverL3;
              SPDLOG_TRACE("  - voxel ({}) -> local ({}) -> conc {}", ix,
                           localPoint, result);
              // replace negative values with zero
              comp.concentration[ix * comp.nSpecies + iSpecies] =
                  result < 0 ? 0 : result;
            }
            ++iElement;
          }
          ++iSpecies;
        }
      }
    }
    for (auto &comp : duneCompartments) {
      // fill in missing pixels with neighbouring value
      for (const auto &[ixMissing, ixNeighbour] : comp.missingVoxels) {
        for (std::size_t iSpecies = 0; iSpecies < comp.nSpecies; ++iSpecies) {
          comp.concentration[ixMissing * comp.nSpecies + iSpecies] =
              comp.concentration[ixNeighbour * comp.nSpecies + iSpecies];
        }
      }
    }
  }

  void initDuneSimCompartments(
      const std::vector<std::unique_ptr<geometry::Compartment>> &comps) {
    duneCompartments.clear();
    std::size_t compIndex{0};
    for (const auto &comp : comps) {
      // TODO: check for empty compartment with dummy species here?
      SPDLOG_DEBUG("compartment: {} - Dune index {}", comp->getId(), compIndex);
      auto imgVolume{comp->getCompartmentImages().volume()};
      auto nPixels{comp->getVoxels().size()};
      SPDLOG_INFO("  - {} pixels", nPixels);
      const auto &compartmentSpeciesNames{speciesNames[comp->getId()]};
      std::size_t nNonConstantSpecies{0};
      for (const auto &speciesName : compartmentSpeciesNames) {
        if (!speciesName.empty()) {
          ++nNonConstantSpecies;
        }
      }
      auto nSpecies{compartmentSpeciesNames.size()};
      SPDLOG_INFO("  - {} species", nSpecies);
      SPDLOG_INFO("    - of which {} non-constant species",
                  nNonConstantSpecies);
      duneCompartments.push_back(
          {comp->getId(),
           compIndex,
           nNonConstantSpecies,
           compartmentSpeciesNames,
           geometry::VoxelIndexer(imgVolume, comp->getVoxels()),
           comp.get(),
           {},
           {},
           std::vector<double>(nPixels * nNonConstantSpecies, 0.0)});
      ++compIndex;
    }
  }

  void updateVoxels() {
    for (auto &comp : duneCompartments) {
      comp.voxels.clear();
      comp.missingVoxels.clear();
      SPDLOG_TRACE("compartment[{}]: {}", comp.index, comp.name);
      const auto &gridview{
          grid->subDomain(static_cast<int>(comp.index)).leafGridView()};
      const auto &qpi{comp.voxelIndexer};
      std::vector<bool> ixAssigned(qpi.size(), false);
      // get local coord for each pixel in each triangle
      std::vector<std::array<double, DuneDimensions>> corners(DuneDimensions +
                                                              1);
      Dune::FieldVector<double, DuneDimensions> duneCoord;
      for (const auto e : elements(gridview)) {
        auto &voxelsInElement = comp.voxels.emplace_back();
        const auto &geo = e.geometry();
        auto ref = Dune::referenceElement(geo);
        for (std::size_t i = 0; i < geo.corners(); ++i) {
          for (std::size_t j = 0; j < DuneDimensions; ++j) {
            corners[i][j] = geo.corner(i)[j];
          }
        }
        auto [pMin, pMax] = getBoundingBox(corners, pixelSize, pixelOrigin);
        SPDLOG_TRACE("  - bounding box ({},{},{}) - ({},{},{})", pMin.p.x(),
                     pMin.p.y(), pMin.z, pMax.p.x(), pMax.p.y(), pMax.z);
        for (int x = pMin.p.x(); x < pMax.p.x() + 1; ++x) {
          for (int y = pMin.p.y(); y < pMax.p.y() + 1; ++y) {
            for (std::size_t z = pMin.z; z < pMax.z + 1; ++z) {
              // global coordinate of center of voxel
              duneCoord[0] =
                  (static_cast<double>(x) + 0.5) * pixelSize.width() +
                  pixelOrigin.p.x();
              duneCoord[1] =
                  (static_cast<double>(y) + 0.5) * pixelSize.height() +
                  pixelOrigin.p.y();
              if constexpr (DuneDimensions == 3) {
                duneCoord[2] =
                    (static_cast<double>(z) + 0.5) * pixelSize.depth() +
                    pixelOrigin.z;
              }
              // as local coordinate for this element
              auto localPoint = e.geometry().local(duneCoord);
              // note: qpi/QImage has (0,0) in top-left corner:
              common::Voxel vox{x, geometryImageSize.height() - 1 - y, z};
              if (auto ix{qpi.getIndex(vox)};
                  ix.has_value() && ref.checkInside(localPoint)) {
                voxelsInElement.push_back({*ix, localPoint});
                ixAssigned[*ix] = true;
              }
            }
          }
        }
        SPDLOG_TRACE("    - found {} voxels", voxelsInElement.size());
      }
      // Deal with voxels that fell outside of mesh (either in a membrane, or
      // where the mesh boundary differs a little from the pixel boundary).
      // For now we just set the value to the nearest voxel from the same
      // compartment which does lie inside an element of the mesh
      for (std::size_t ix = 0; ix < ixAssigned.size(); ++ix) {
        if (!ixAssigned[ix]) {
          SPDLOG_DEBUG("voxel {} not in a triangle", ix);
          // find a neighbouring valid pixel
          auto ixNeighbour = getIxValidNeighbour(ix, ixAssigned, comp.geometry);
          SPDLOG_DEBUG("  -> using concentration from voxel {}", ixNeighbour);
          comp.missingVoxels.push_back({ix, ixNeighbour});
        }
      }
    }
  }

  void setInitial(const DuneConverter &dc) {
    SPDLOG_INFO("Initial condition functions:");
    auto initialConditionFunctions{
        makeModelDuneFunctions<typename Model::Grid,
                               typename Model::GridFunction>(dc, *grid)};
    for (const auto &[k, v] : initialConditionFunctions) {
      SPDLOG_INFO("  - {}", k);
    }
    model->interpolate(*state, initialConditionFunctions);
    SPDLOG_INFO("end of interpolate");
  }
};

} // namespace sme::simulate
