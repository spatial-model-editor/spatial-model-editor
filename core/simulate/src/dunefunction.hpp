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
#include <dune/pdelab/common/function.hh>
#include <memory>
#include <vector>

namespace Dune {
template <typename F, int n> class FieldVector;
}

namespace sme::simulate {

template <typename GV>
class GridFunction
    : public Dune::PDELab::GridFunctionBase<
          Dune::PDELab::GridFunctionTraits<GV, double, 1,
                                           Dune::FieldVector<double, 1>>,
          GridFunction<GV>> {
public:
  using Traits = Dune::PDELab::GridFunctionTraits<GV, double, 1,
                                                  Dune::FieldVector<double, 1>>;
  GridFunction(const common::VoxelF &origin, const common::VolumeF &voxelSize,
               const common::Volume &imageSize,
               const std::vector<double> &concentration)
      : origin{origin}, voxelSize{voxelSize}, imageSize{imageSize},
        c(concentration) {
    SPDLOG_TRACE("  - {}x{} pixels", imageSize.width(), imageSize.height());
    SPDLOG_TRACE("  - {}x{} pixel size", voxelSize.width(), voxelSize.height());
    SPDLOG_TRACE("  - ({},{}) origin", origin.p.x(), origin.p.y());
  }
  void set_time([[maybe_unused]] double t) {}
  template <typename Element, typename Domain>
  void evaluate(const Element &elem, const Domain &localPos,
                typename Traits::RangeType &result) const {
    if (c.empty()) {
      // dummy species, just return 0 everywhere
      result = 0;
      return;
    }
    SPDLOG_TRACE("localPos ({},{})", localPos[0], localPos[1]);
    auto globalPos = elem.geometry().global(localPos);
    SPDLOG_TRACE("globalPos ({},{})", globalPos[0], globalPos[1]);
    // get nearest pixel to physical point
    auto ix = std::clamp(
        static_cast<int>((globalPos[0] - origin.p.x()) / voxelSize.width()), 0,
        imageSize.width() - 1);
    auto iy = std::clamp(
        static_cast<int>((globalPos[1] - origin.p.y()) / voxelSize.height()), 0,
        imageSize.height() - 1);
    result = c[static_cast<std::size_t>(ix + imageSize.width() * iy)];
    SPDLOG_TRACE("pixel ({},{})", ix, iy);
    SPDLOG_TRACE("conc {}", result);
  }

private:
  common::VoxelF origin;
  common::VolumeF voxelSize;
  common::Volume imageSize;
  std::vector<double> c;
};

template <class GV>
auto makeCompartmentDuneFunctions(const DuneConverter &dc,
                                  std::size_t compIndex) {
  std::vector<std::shared_ptr<GridFunction<GV>>> functions;
  const auto &concentrations = dc.getConcentrations()[compIndex];
  std::size_t nSpecies{concentrations.size()};
  functions.reserve(nSpecies);
  SPDLOG_TRACE("compartment {}", compIndex);
  SPDLOG_TRACE("  - contains {} species", nSpecies);
  for (const auto &concentration : concentrations) {
    functions.emplace_back(std::make_shared<GridFunction<GV>>(
        dc.getOrigin(), dc.getVoxelSize(), dc.getImageSize(), concentration));
  }
  return functions;
}

template <class GV> auto makeModelDuneFunctions(const DuneConverter &dc) {
  std::vector<std::vector<std::shared_ptr<GridFunction<GV>>>> functions;
  const auto &concentrations = dc.getConcentrations();
  functions.reserve(concentrations.size());
  for (std::size_t i = 0; i < concentrations.size(); ++i) {
    functions.push_back(makeCompartmentDuneFunctions<GV>(dc, i));
  }
  return functions;
}

} // namespace sme::simulate
