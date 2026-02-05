// Shared helpers for DUNE cell data mapping

#pragma once

#include "dune_headers.hpp"
#include "sme/duneconverter.hpp"
#include "sme/voxel.hpp"
#include <algorithm>
#include <dune/copasi/grid/cell_data.hh>
#include <dune/grid/common/mcmgmapper.hh>
#include <dune/grid/common/rangegenerators.hh>
#include <fmt/core.h>
#include <memory>
#include <stdexcept>
#include <unordered_map>
#include <vector>

namespace sme::simulate {

template <typename GridView>
std::vector<std::size_t>
makeElementToVoxel(const common::VoxelF &origin, const common::VolumeF &voxel,
                   const common::Volume &vol, const GridView &gridView) {
  Dune::MultipleCodimMultipleGeomTypeMapper<GridView> mapper(
      gridView, Dune::mcmgElementLayout());
  std::vector<std::size_t> elementToVoxel(mapper.size(), 0);
  for (const auto &cell : elements(gridView)) {
    auto idx = mapper.index(cell);
    const auto center = cell.geometry().center();
    auto ix = common::toVoxelIndex(center[0], origin.p.x(), voxel.width(),
                                   vol.width());
    auto iy = common::toVoxelIndex(center[1], origin.p.y(), voxel.height(),
                                   vol.height());
    int iz = 0;
    if constexpr (GridView::dimensionworld == 3) {
      iz = common::toVoxelIndex(center[2], origin.z, voxel.depth(),
                                static_cast<int>(vol.depth()));
    }
    elementToVoxel[idx] = common::voxelArrayIndex(
        vol, ix, iy, static_cast<std::size_t>(iz), false);
  }
  return elementToVoxel;
}

template <typename GridView>
std::unordered_map<std::string, std::vector<double>>
makeDiffusionCellValuesForGridView(const DuneConverter &dc,
                                   const GridView &gridView) {
  const auto &arrays = dc.getDiffusionConstantArrays();
  if (arrays.empty()) {
    return {};
  }
  const auto origin = dc.getOrigin();
  const auto voxel = dc.getVoxelSize();
  const auto vol = dc.getImageSize();
  const auto nVoxels = vol.nVoxels();
  const auto elementToVoxel = makeElementToVoxel(origin, voxel, vol, gridView);

  std::unordered_map<std::string, std::vector<double>> valuesByName;
  valuesByName.reserve(arrays.size());
  for (const auto &[name, data] : arrays) {
    if (data.size() != nVoxels) {
      auto errorMsg =
          fmt::format("Diffusion array '{}' has size {} but expected {} voxels",
                      name, data.size(), nVoxels);
      SPDLOG_ERROR("{}", errorMsg);
      throw std::runtime_error(errorMsg);
    }
    std::vector<double> values(elementToVoxel.size(), 0.0);
    for (std::size_t i = 0; i < elementToVoxel.size(); ++i) {
      values[i] = data[elementToVoxel[i]];
    }
    valuesByName.emplace(name, std::move(values));
  }
  return valuesByName;
}

template <typename GridView>
std::shared_ptr<Dune::Copasi::CellData<GridView, double>>
makeDiffusionCellDataForGridView(const DuneConverter &dc,
                                 const GridView &gridView) {
  auto valuesByName = makeDiffusionCellValuesForGridView(dc, gridView);
  if (valuesByName.empty()) {
    return nullptr;
  }
  auto cell_data =
      std::make_shared<Dune::Copasi::CellData<GridView, double>>(gridView);
  cell_data->reserve(valuesByName.size());
  for (const auto &[name, values] : valuesByName) {
    cell_data->addData(name, values);
  }
  return cell_data;
}

} // namespace sme::simulate
