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
    SPDLOG_TRACE("  - {}x{}x{} voxels", vol.width(), vol.height(), vol.depth());
    SPDLOG_TRACE("  - {}x{}x{} voxel size", voxel.width(), voxel.height(),
                 voxel.height());
    SPDLOG_TRACE("  - ({},{},{}) origin", origin.p.x(), origin.p.y(), origin.z);
  }
  double operator()(Domain globalPos) const {
    SPDLOG_TRACE("globalPos ({})", globalPos);
    if (c.empty()) {
      // dummy species, just return 0 everywhere
      return 0;
    }
    // get nearest voxel to physical point
    auto ix = common::toVoxelIndex(globalPos[0], origin.p.x(), voxel.width(),
                                   vol.width());
    auto iy = common::toVoxelIndex(globalPos[1], origin.p.y(), voxel.height(),
                                   vol.height());
    int iz = 0;
    if constexpr (Domain::size() == 3) {
      iz = common::toVoxelIndex(globalPos[2], origin.z, voxel.depth(),
                                static_cast<int>(vol.depth()));
    }
    auto ci =
        common::voxelArrayIndex(vol, ix, iy, static_cast<std::size_t>(iz));
    SPDLOG_TRACE("  -> voxel ({},{},{})", ix, iy, iz);
    SPDLOG_TRACE("  -> conc[{}] = {}", ci, c[ci]);
    return c[ci];
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
  std::unordered_map<std::string, GridFunction> functions;
  SPDLOG_TRACE("Creating DuneGridFunctions:");
  std::size_t subdomain{0};
  for (const auto &compartmentId : dc.getCompartmentNames()) {
    SPDLOG_TRACE("Compartment {} {}", subdomain, compartmentId);
    auto gridView{
        grid.subDomain(static_cast<unsigned int>(subdomain)).leafGridView()};
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
