//  - makeDuneFunctions(): create dune-copasi grid functions
//  - they evaluate the initial concentrations for all species in model
//  - based on pdelab_expression_adapter.hh from dune-copasi
//  - Note: ensure any changes to pdelab_expression_adapter.hh in future
//  versions of dune-copasi are taken into account here if relevant

#pragma once

#include "sme/duneconverter.hpp"
#include "sme/logger.hpp"
#include "sme/voxel.hpp"
#include <QPoint>
#include <algorithm>
#include <cstddef>
#include <dune/functions/gridfunctions/gridviewfunction.hh>
#include <memory>
#include <vector>

namespace Dune {
template <typename F, int n> class FieldVector;
}

namespace sme::simulate {

template <typename Domain> class SmeGridFunction {
public:
  SmeGridFunction(const common::VoxelF &physicalOrigin,
                  const common::VolumeF &voxelVolume,
                  const common::Volume &imageVolume,
                  const std::vector<double> &concentration)
      : origin{physicalOrigin}, voxel{voxelVolume}, vol{imageVolume},
        c(concentration) {
    SPDLOG_TRACE("  - {}x{} pixels", vol.width(), vol.height());
    SPDLOG_TRACE("  - {}x{} pixel size", voxel.width(), voxel.height());
    SPDLOG_TRACE("  - ({},{}) origin", origin.p.x(), origin.p.y());
  }
  double operator()(Domain globalPos) const {
    SPDLOG_TRACE("globalPos ({},{})", globalPos[0], globalPos[1]);
    if (c.empty()) {
      // dummy species, just return 0 everywhere
      return 0;
    }
    // get nearest pixel to physical point
    auto ix = std::clamp(
        static_cast<int>((globalPos[0] - origin.p.x()) / voxel.width()), 0,
        vol.width() - 1);
    auto iy = std::clamp(
        static_cast<int>((globalPos[1] - origin.p.y()) / voxel.height()), 0,
        vol.height() - 1);
    SPDLOG_TRACE("pixel ({},{})", ix, iy);
    SPDLOG_TRACE("conc {}", c[static_cast<std::size_t>(ix + vol.width() * iy)]);
    return c[static_cast<std::size_t>(ix + vol.width() * iy)];
  }

private:
  common::VoxelF origin;
  common::VolumeF voxel;
  common::Volume vol;
  std::vector<double> c;
};

template <typename Grid, typename GridFunction>
std::unordered_map<std::string, GridFunction>
makeModelDuneFunctions(const DuneConverter &dc, const Grid &grid) {
  //  using GV = typename Grid::LeafGridView;
  //  using EntitySet = Dune::Functions::GridViewEntitySet<GV, 0>;
  //  using Domain = typename EntitySet::GlobalCoordinate;
  std::unordered_map<std::string, GridFunction> functions;
  SPDLOG_TRACE("Creating DuneGridFunctions:");
  std::size_t subdomain{0};
  for (const auto &compartmentId : dc.getCompartmentNames()) {
    SPDLOG_TRACE("Compartment {} {}", subdomain, compartmentId);
    auto gridView{grid.subDomain(static_cast<int>(subdomain)).leafGridView()};
    using Domain = typename decltype(gridView)::template Codim<
        0>::Geometry::GlobalCoordinate;
    for (const auto &speciesName : dc.getSpeciesNames().at(compartmentId)) {
      if (!speciesName.empty()) {
        SPDLOG_TRACE("  - {}", speciesName);
        auto func{SmeGridFunction<Domain>(
            dc.getOrigin(), dc.getVoxelSize(), dc.getImageSize(),
            dc.getConcentrations().at(speciesName))};
        functions[fmt::format("{}.{}", compartmentId, speciesName)] =
            Dune::Functions::makeAnalyticGridViewFunction(func, gridView);
      }
    }
    ++subdomain;
  }
  return functions;
}

} // namespace sme::simulate
