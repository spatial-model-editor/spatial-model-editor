#pragma once

#include "sme/image_stack.hpp"
#include <QString>
#include <array>
#include <optional>
#include <vector>

namespace sme::mesh {

struct GMSHTetrahedron {
  std::array<std::size_t, 4> vertexIndices{};
  int physicalTag{1};
};

struct GMSHMesh {
  std::vector<common::VoxelF> vertices{};
  std::vector<GMSHTetrahedron> tetrahedra{};
};

/**
 * @brief Read a Gmsh v2 mesh into a reusable in-memory mesh object
 *
 * @param[in] filename path to a gmsh v2 ascii ``.msh`` file
 * @returns parsed mesh, or empty optional on failure
 */
[[nodiscard]] std::optional<GMSHMesh> readGMSHMesh(const QString &filename);

/**
 * @brief Voxelize an in-memory GMSHMesh into an ImageStack
 *
 * Tetrahedra are grouped by physical tag and each compartment is assigned a
 * default color from common::indexedColors() in ascending physical-tag order.
 *
 * @param[in] mesh mesh object from readGMSHMesh
 * @param[in] maxVoxelsPerDimension max voxels for the largest image dimension
 * @param[in] includeBackground if false, unassigned voxels are filled using
 * nearest assigned compartment voxels
 * @returns an RGB32 ImageStack, or an empty ImageStack on failure
 */
[[nodiscard]] common::ImageStack
voxelizeGMSHMesh(const GMSHMesh &mesh, int maxVoxelsPerDimension = 50,
                 bool includeBackground = true);

} // namespace sme::mesh
