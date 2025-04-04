#pragma once

#include "sme/image_stack.hpp"
#include "sme/mesh_types.hpp"
#include <QImage>
#include <QPointF>
#include <QRgb>
#include <QSize>
#include <QString>
#include <QVector4D>
#include <array>
#include <cstddef>
#include <memory>
#include <string>
#include <utility>
#include <vector>

namespace CGAL {
class Image_3;
}

namespace sme::mesh {

using QTetrahedronF = std::array<sme::common::VoxelF, 4>;
using TriangleVertexIndices = std::array<std::size_t, 3>;
using TetrahedronVertexIndices = std::array<std::size_t, 4>;

class Mesh3d {
private:
  // input data
  std::unique_ptr<CGAL::Image_3> image3_;
  std::vector<std::size_t> compartmentMaxCellVolume_;
  // generated data
  std::vector<sme::common::VoxelF> vertices_;
  std::vector<std::vector<QTetrahedronF>> tetrahedra_;
  std::vector<std::vector<TetrahedronVertexIndices>> tetrahedronVertexIndices_;
  std::vector<std::vector<TriangleVertexIndices>>
      membraneTriangleVertexIndices_;
  std::vector<std::uint8_t> labelToCompartmentIndex_;
  std::vector<std::uint8_t> compartmentsToMembraneIndex_;
  bool validMesh_{false};
  std::vector<QRgb> colors_;
  std::string errorMessage_{};
  void constructMesh();

public:
  Mesh3d();
  /**
   * @brief Constructs a 3d mesh from the supplied ImageStack
   *
   * @param[in] imageStack the segmented geometry ImageStack
   * @param[in] maxCellVolume the max volume (in voxels) of a cell for each
   * compartment
   * @param[in] voxelSize the physical size of a voxel
   * @param[in] originPoint the physical location of the ``(0,0,0)`` voxel
   * @param[in] compartmentColors the colors of compartments in the image
   * @param[in] membraneIdColorPairs the ids and color-pairs of membranes in the
   * image
   */
  explicit Mesh3d(
      const sme::common::ImageStack &imageStack,
      std::vector<std::size_t> maxCellVolume = {},
      const common::VolumeF &voxelSize = {1.0, 1.0, 1.0},
      const common::VoxelF &originPoint = {0.0, 0.0, 0.0},
      const std::vector<QRgb> &compartmentColors = {},
      const std::vector<std::pair<std::string, std::pair<QRgb, QRgb>>>
          &membraneIdColorPairs = {});
  ~Mesh3d();
  /**
   * @brief Returns true if the mesh is valid
   */
  [[nodiscard]] bool isValid() const;
  /**
   * @brief Returns an error message if the mesh is invalid
   */
  [[nodiscard]] const std::string &getErrorMessage() const;
  /**
   * @brief Set the maximum allowed cell volume in voxels for a given
   * compartment
   *
   * @param[in] compartmentIndex the index of the compartment
   * @param[in] maxTriangleArea the maximum allowed cell volume in voxels
   */
  void setCompartmentMaxCellVolume(std::size_t compartmentIndex,
                                   std::size_t maxCellVolume);
  /**
   * @brief Get the maximum allowed cell volume for a given compartment
   *
   * @param[in] compartmentIndex the index of the compartment
   */
  [[nodiscard]] std::size_t
  getCompartmentMaxCellVolume(std::size_t compartmentIndex) const;
  /**
   * @brief The maximum allowed cell volume in voxels for each compartment in
   * the mesh
   */
  [[nodiscard]] const std::vector<std::size_t> &
  getCompartmentMaxCellVolume() const;
  /**
   * @brief The physical volume and origin to use
   *
   * The boundary lines and mesh use pixel units internally, and are rescaled
   * to physical values using the supplied physical origin and pixel width.
   *
   * @param[in] voxelSize the physical size of a voxel
   * @param[in] originPoint the physical location of the ``(0,0,0)`` voxel
   */
  void setPhysicalGeometry(const common::VolumeF &voxelSize,
                           const common::VoxelF &originPoint = {0.0, 0.0, 0.0});
  /**
   * @brief The physical mesh vertices as a vector of VoxelF
   */
  [[nodiscard]] const std::vector<sme::common::VoxelF> &getVertices() const;
  /**
   * @brief The physical mesh vertices as a flat array of doubles
   *
   * For saving to the SBML document.
   */
  [[nodiscard]] std::vector<double> getVerticesAsFlatArray() const;
  /**
   *
   * @return number of compartments in mesh.
   */
  [[nodiscard]] std::size_t getNumberOfCompartments() const;
  /**
   *
   * @return number of membranes in mesh.
   */
  [[nodiscard]] std::size_t getNumberOfMembranes() const;
  /**
   * @brief The mesh tetrahedron indices as a flat array of ints
   *
   * For saving to the SBML document.
   */
  [[nodiscard]] std::vector<int>
  getTetrahedronIndicesAsFlatArray(std::size_t compartmentIndex) const;
  /**
   * @brief The mesh tetrahedron indices
   *
   * The indices of the vertices of each tetrahedron used in the mesh for each
   * compartment
   */
  [[nodiscard]] const std::vector<std::vector<TetrahedronVertexIndices>> &
  getTetrahedronIndices() const;
  /**
   * @brief The mesh tetrahedron indices
   *
   * The indices of the vertices of each triangle on the boundary between
   * compartments c1 and c2
   */
  [[nodiscard]] const std::vector<std::vector<TriangleVertexIndices>> &
  getMembraneTriangleIndices() const;
  /**
   * @brief The mesh in GMSH format
   *
   * @returns the mesh in GMSH format
   */
  [[nodiscard]] QString getGMSH() const;

  /**
   * @brief Get the colors of the compartments and membranes
   *
   */
  [[nodiscard]] const std::vector<QRgb> &getColors() const;

  /**
   * @brief Set the colors of the compartments and membranes
   *
   */
  void
  setColors(const std::vector<QRgb> &compartmentColors,
            const std::vector<std::pair<std::string, std::pair<QRgb, QRgb>>>
                &membraneIdColorPairs);

  static constexpr std::size_t dim = 3;
};

} // namespace sme::mesh
