#pragma once

#include "sme/feature_options.hpp"
#include "sme/geometry.hpp"
#include "sme/voxel.hpp"
#include <cstddef>
#include <vector>

namespace sme::simulate {

/**
 * @brief Compute per-voxel ROI region indices from ROI settings.
 *
 * @param roi ROI settings defining how voxels map to ROI regions.
 * @param comp Compartment providing voxel positions and neighbors.
 * @param origin Physical origin of the geometry.
 * @param voxelSize Physical size of each voxel.
 * @returns Vector of size nVoxels. Value = region index (0..numRegions),
 *          where 0 means excluded.
 */
std::vector<std::size_t> computeVoxelRegions(const RoiSettings &roi,
                                             const geometry::Compartment &comp,
                                             const common::VoxelF &origin,
                                             const common::VolumeF &voxelSize);

/**
 * @brief Depth regions via iterative erosion from outer boundary.
 *
 * Boundary voxels (where any neighbor == self via Neumann BC) = region 1.
 * Each subsequent region is peeled inward.
 *
 * @param comp Compartment providing voxels and neighbor indices.
 * @param numRegions Number of depth regions.
 * @param regionThickness Number of voxel layers per depth region.
 * @returns Vector of size nVoxels with region indices (1..numRegions).
 *          Voxels deeper than ``numRegions * regionThickness`` are excluded
 *          from the ROI.
 *          0 means excluded.
 */
std::vector<std::size_t> computeDepthRegions(const geometry::Compartment &comp,
                                             std::size_t numRegions,
                                             std::size_t regionThickness = 1);

/**
 * @brief Axis slice regions across the compartment extent.
 *
 * Axis values: x=0, y=1, z=2. The y axis uses physical image orientation,
 * matching analytic ROI coordinates.
 *
 * @param comp Compartment providing voxel positions.
 * @param numRegions Number of axis bands.
 * @param axis Axis index: x=0, y=1, z=2.
 * @returns Vector of size nVoxels with region indices (1..numRegions).
 *          0 means excluded.
 */
std::vector<std::size_t>
computeAxisSliceRegions(const geometry::Compartment &comp,
                        std::size_t numRegions, int axis = 0);

/**
 * @brief Apply reduction operation to concentrations in a specific region.
 *
 * @param op Reduction operation.
 * @param concs Concentration values (one per voxel).
 * @param voxelRegions ROI region indices (one per voxel, 0 = excluded).
 * @param targetRegion Region index to reduce over.
 * @returns Scalar result of the reduction.
 */
double applyReduction(ReductionOp op, const std::vector<double> &concs,
                      const std::vector<std::size_t> &voxelRegions,
                      std::size_t targetRegion);

/**
 * @brief Evaluate one feature at one timepoint.
 *
 * @param feature Feature definition.
 * @param concs Concentration values for the species (one per voxel).
 * @param voxelRegions Pre-computed ROI region indices for this feature.
 * @returns Vector of ``numRegions`` scalars (one per region).
 */
std::vector<double>
evaluateFeature(const FeatureDefinition &feature,
                const std::vector<double> &concs,
                const std::vector<std::size_t> &voxelRegions);

/**
 * @brief Number of regions for a given ROI settings.
 */
std::size_t getNumRegions(const RoiSettings &roi);

} // namespace sme::simulate
