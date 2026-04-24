#include "sme/feature_eval.hpp"
#include "sme/feature_eval_quantiles.hpp"
#include "sme/feature_options.hpp"
#include "sme/logger.hpp"
#include "sme/symbolic.hpp"
#include <algorithm>

namespace sme::simulate {

static constexpr std::size_t excludedRegion = 0;

std::size_t getNumRegions(const RoiSettings &roi) { return roi.numRegions; }

static std::vector<std::size_t> computeAnalyticRegions(
    const RoiSettings &roi, const geometry::Compartment &comp,
    const common::VoxelF &origin, const common::VolumeF &voxelSize) {
  std::vector<std::size_t> regions(comp.nVoxels(), excludedRegion);
  if (roi.numRegions == 0 || roi.expression.empty()) {
    return regions;
  }
  common::Symbolic sym(roi.expression, {"x", "y", "z"});
  if (!sym.isValid()) {
    SPDLOG_WARN("Invalid ROI region expression: {}", roi.expression);
    return regions;
  }
  sym.compile();
  const auto &voxels = comp.getVoxels();
  const auto &imgSize = comp.getImageSize();
  std::vector<double> result(1);
  std::vector<double> vars(3);
  for (std::size_t i = 0; i < comp.nVoxels(); ++i) {
    const auto &v = voxels[i];
    vars[0] =
        origin.p.x() + (static_cast<double>(v.p.x()) + 0.5) * voxelSize.width();
    vars[1] = origin.p.y() +
              (static_cast<double>(imgSize.height() - 1 - v.p.y()) + 0.5) *
                  voxelSize.height();
    vars[2] = origin.z + (static_cast<double>(v.z) + 0.5) * voxelSize.depth();
    sym.eval(result, vars);
    const auto region = static_cast<int>(result[0]);
    if (region > 0 && static_cast<std::size_t>(region) <= roi.numRegions) {
      regions[i] = static_cast<std::size_t>(region);
    }
  }
  return regions;
}

static std::vector<std::size_t>
computeImageRegions(const RoiSettings &roi, const geometry::Compartment &comp) {
  std::vector<std::size_t> regions(comp.nVoxels(), excludedRegion);
  if (roi.numRegions == 0 || roi.regionImage.empty()) {
    return regions;
  }
  const auto &voxels = comp.getVoxels();
  const auto &imgSize = comp.getImageSize();
  for (std::size_t i = 0; i < comp.nVoxels(); ++i) {
    const auto &v = voxels[i];
    const auto imgIndex =
        common::voxelArrayIndex(imgSize, v.p.x(), v.p.y(), v.z);
    if (imgIndex < roi.regionImage.size()) {
      const auto region = roi.regionImage[imgIndex];
      if (region <= roi.numRegions) {
        regions[i] = region;
      }
    }
  }
  return regions;
}

std::vector<std::size_t> computeDepthRegions(const geometry::Compartment &comp,
                                             std::size_t numRegions,
                                             std::size_t regionThickness) {
  std::vector<std::size_t> regions(comp.nVoxels(), excludedRegion);
  if (comp.nVoxels() == 0 || numRegions == 0) {
    return regions;
  }
  regionThickness = std::max<std::size_t>(regionThickness, 1);
  const auto &imgSize = comp.getImageSize();
  const bool checkX = imgSize.width() > 1;
  const bool checkY = imgSize.height() > 1;
  const bool checkZ = imgSize.depth() > 1;
  if (!checkX && !checkY && !checkZ) {
    std::fill(regions.begin(), regions.end(), 1);
    return regions;
  }
  for (std::size_t i = 0; i < comp.nVoxels(); ++i) {
    if ((checkX && (comp.up_x(i) == i || comp.dn_x(i) == i)) ||
        (checkY && (comp.up_y(i) == i || comp.dn_y(i) == i)) ||
        (checkZ && (comp.up_z(i) == i || comp.dn_z(i) == i))) {
      regions[i] = 1;
    }
  }
  const auto maxLayer = numRegions * regionThickness;
  std::vector<std::size_t> newRegion;
  for (std::size_t layer = 2; layer <= maxLayer; ++layer) {
    newRegion.clear();
    for (std::size_t i = 0; i < comp.nVoxels(); ++i) {
      if (regions[i] != excludedRegion) {
        continue;
      }
      const auto checkNeighbor = [&](std::size_t ni) {
        return ni != i && regions[ni] != excludedRegion;
      };
      if (checkNeighbor(comp.up_x(i)) || checkNeighbor(comp.dn_x(i)) ||
          checkNeighbor(comp.up_y(i)) || checkNeighbor(comp.dn_y(i)) ||
          checkNeighbor(comp.up_z(i)) || checkNeighbor(comp.dn_z(i))) {
        newRegion.push_back(i);
      }
    }
    const auto region = 1 + (layer - 1) / regionThickness;
    for (auto idx : newRegion) {
      regions[idx] = region;
    }
  }
  return regions;
}

std::vector<std::size_t>
computeAxisSliceRegions(const geometry::Compartment &comp,
                        std::size_t numRegions, int axis) {
  std::vector<std::size_t> regions(comp.nVoxels(), excludedRegion);
  if (comp.nVoxels() == 0 || numRegions == 0) {
    return regions;
  }
  axis = std::clamp(axis, 0, 2);
  const auto &imgSize = comp.getImageSize();
  const auto coordinate = [&](const common::Voxel &v) {
    switch (axis) {
    case 1:
      return imgSize.height() - 1 - v.p.y();
    case 2:
      return static_cast<int>(v.z);
    default:
      return v.p.x();
    }
  };
  const auto &voxels = comp.getVoxels();
  auto [minVoxel, maxVoxel] =
      std::ranges::minmax_element(voxels, {}, coordinate);
  const auto minCoordinate = coordinate(*minVoxel);
  const auto extent =
      static_cast<std::size_t>(coordinate(*maxVoxel) - minCoordinate + 1);
  for (std::size_t i = 0; i < comp.nVoxels(); ++i) {
    const auto distance =
        static_cast<std::size_t>(coordinate(voxels[i]) - minCoordinate);
    regions[i] = std::min(numRegions, 1 + distance * numRegions / extent);
  }
  return regions;
}

std::vector<std::size_t> computeVoxelRegions(const RoiSettings &roi,
                                             const geometry::Compartment &comp,
                                             const common::VoxelF &origin,
                                             const common::VolumeF &voxelSize) {
  switch (roi.roiType) {
  case RoiType::Analytic:
    return computeAnalyticRegions(roi, comp, origin, voxelSize);
  case RoiType::Image:
    return computeImageRegions(roi, comp);
  case RoiType::Depth:
    return computeDepthRegions(
        comp, roi.numRegions,
        static_cast<std::size_t>(std::max(
            1, getRoiParameterInt(roi, roi_param::depthThicknessVoxels, 1))));
  case RoiType::AxisSlices:
    return computeAxisSliceRegions(comp, roi.numRegions,
                                   getRoiParameterInt(roi, roi_param::axis, 0));
  }
  return std::vector<std::size_t>(comp.nVoxels(), excludedRegion);
}

double applyReduction(ReductionOp op, const std::vector<double> &concs,
                      const std::vector<std::size_t> &voxelRegions,
                      std::size_t targetRegion) {
  double result = 0.0;
  std::size_t count = 0;
  bool first = true;
  double q = 0.0;
  bool is_quantile = false;
  switch (op) {
  case ReductionOp::FirstQuantile:
    is_quantile = true;
    q = 0.25;
  case ReductionOp::Median:
    is_quantile = true;
    q = 0.5;
  case ReductionOp::ThirdQuantile:
    is_quantile = true;
    q = 0.75;
  default:
    // do nothing
    q = 0.0;
    is_quantile = false;
  }

  if (is_quantile) {
    if (concs.size() > 0) {
      result = Quantile(q)(concs);
    }
    return result;
  } else {

    for (std::size_t i = 0; i < concs.size(); ++i) {
      if (voxelRegions[i] != targetRegion) {
        continue;
      }
      const double c = concs[i];
      if (first) {
        if (op == ReductionOp::Min || op == ReductionOp::Max) {
          result = c;
        }
        first = false;
      }
      switch (op) {
      case ReductionOp::Average:
      case ReductionOp::Sum:
        result += c;
        break;
      case ReductionOp::Min:
        result = std::min(result, c);
        break;
      case ReductionOp::Max:
        result = std::max(result, c);
        break;
      default:
        continue;
      }
      ++count;
    }
    if (op == ReductionOp::Average && count > 0) {
      result /= static_cast<double>(count);
    }
    if (first && (op == ReductionOp::Min || op == ReductionOp::Max)) {
      result = 0.0;
    }
    return result;
  }
}

std::vector<double>
evaluateFeature(const FeatureDefinition &feature,
                const std::vector<double> &concs,
                const std::vector<std::size_t> &voxelRegions) {
  const auto numRegions = getNumRegions(feature.roi);
  std::vector<double> results(numRegions, 0.0);
  std::vector<std::size_t> counts(numRegions, 0);
  std::vector<bool> initialized(numRegions, false);
  const auto op = feature.reduction;
  for (std::size_t i = 0; i < concs.size(); ++i) {
    const auto region = voxelRegions[i];
    if (region == excludedRegion || region > numRegions) {
      continue;
    }
    const auto regionIndex = region - 1;
    const double c = concs[i];
    if (!initialized[regionIndex]) {
      initialized[regionIndex] = true;
      if (op == ReductionOp::Min || op == ReductionOp::Max) {
        results[regionIndex] = c;
      }
    }
    switch (op) {
    case ReductionOp::Average:
    case ReductionOp::Sum:
      results[regionIndex] += c;
      break;
    case ReductionOp::Min:
      results[regionIndex] = std::min(results[regionIndex], c);
      break;
    case ReductionOp::Max:
      results[regionIndex] = std::max(results[regionIndex], c);
      break;
    }
    ++counts[regionIndex];
  }
  for (std::size_t regionIndex = 0; regionIndex < numRegions; ++regionIndex) {
    if (op == ReductionOp::Average && counts[regionIndex] > 0) {
      results[regionIndex] /= static_cast<double>(counts[regionIndex]);
    }
  }
  return results;
}

} // namespace sme::simulate
