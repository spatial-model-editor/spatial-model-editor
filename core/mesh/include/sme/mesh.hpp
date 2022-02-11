#pragma once

#include "sme/mesh_types.hpp"
#include <QImage>
#include <QPointF>
#include <QRgb>
#include <QSize>
#include <QString>
#include <array>
#include <cstddef>
#include <memory>
#include <string>
#include <utility>
#include <vector>

namespace sme::mesh {

class Boundaries;

/**
 * @brief Constructs a triangular mesh from a geometry image
 *
 * Given a segmented geometry image, and the colours that correspond to
 * compartments in the image, the compartment boundaries are identified and
 * simplified to a set of connected straight lines, and the resulting PLSG is
 * triangulated to give a triangular mesh.
 *
 * The mesh is provided as an image, in GMSH format, and as flat arrays of
 * indices and vertices for SBML.
 *
 * The number of points used for each boundary line and the maximum triangle
 * area allowed for each compartment can then be adjusted.
 *
 */
class Mesh {
private:
  // input data
  QImage img;
  QPointF origin;
  double pixel{};
  std::vector<std::size_t> boundaryMaxPoints;
  std::vector<std::size_t> compartmentMaxTriangleArea;
  // generated data
  std::unique_ptr<Boundaries> boundaries;
  std::vector<std::vector<QPointF>> compartmentInteriorPoints;
  std::vector<QPointF> vertices;
  std::size_t nTriangles{};
  std::vector<std::vector<QTriangleF>> triangles;
  std::vector<std::vector<TriangulateTriangleIndex>> triangleIndices;
  bool validMesh{true};
  std::string errorMessage{};
  // convert point in pixel units to point in physical units
  [[nodiscard]] QPointF
  pixelPointToPhysicalPoint(const QPointF &pixelPoint) const noexcept;
  void constructMesh();

public:
  Mesh();
  /**
   * @brief Constructs a mesh from the supplied image
   *
   * @param[in] image the segmented geometry image
   * @param[in] maxPoints the max points allowed for each boundary line
   * @param[in] maxTriangleArea the max triangle area allowed for each
   *    compartment
   * @param[in] pixelWidth the physical width of a pixel
   * @param[in] originPoint the physical location of the ``(0,0)`` pixel
   * @param[in] compartmentColours the colours of compartments in the image
   */
  explicit Mesh(const QImage &image, std::vector<std::size_t> maxPoints = {},
                std::vector<std::size_t> maxTriangleArea = {},
                double pixelWidth = 1.0,
                const QPointF &originPoint = QPointF(0, 0),
                const std::vector<QRgb> &compartmentColours = {},
                std::size_t boundarySimplificationType = 0);
  ~Mesh();
  /**
   * @brief Returns true if the mesh is valid
   */
  [[nodiscard]] bool isValid() const;
  /**
   * @brief Returns an error message if the mesh is invalid
   */
  [[nodiscard]] const std::string &getErrorMessage() const;
  /**
   * @brief The number of boundary lines in the mesh
   */
  [[nodiscard]] std::size_t getNumBoundaries() const;
  /**
   * @brief Get the type of boundary simplification
   */
  [[nodiscard]] std::size_t getBoundarySimplificationType() const;
  /**
   * @brief Set the type of boundary simplification
   */
  void setBoundarySimplificationType(std::size_t boundarySimplificationType);
  /**
   * @brief Set the maximum number of allowed points
   *
   * If the boundary simplification type is one where each boundary line can be
   * independently simplified, maxPoints applies only to the specified boundary
   * line, and only this boundary line is potentially altered by this method.
   *
   * If the boundary simplification type is a topology-preserving type that
   * simplifies all boundaries simultaneously, maxPoints is the total number of
   * allowed points summed over all boundaries, and all boundary lines can
   * potentially be altered by this method.
   *
   * @param[in] boundaryIndex the index of the boundary
   * @param[in] maxPoints the maximum number of points allowed
   */
  void setBoundaryMaxPoints(std::size_t boundaryIndex, std::size_t maxPoints);
  /**
   * @brief Get the maximum number of allowed points for a given boundary
   *
   * @param[in] boundaryIndex the index of the boundary
   */
  [[nodiscard]] std::size_t
  getBoundaryMaxPoints(std::size_t boundaryIndex) const;
  /**
   * @brief The maximum allowed points for each boundary in the mesh
   */
  [[nodiscard]] std::vector<std::size_t> getBoundaryMaxPoints() const;
  /**
   * @brief Set the maximum allowed triangle area for a given compartment
   *
   * @param[in] compartmentIndex the index of the compartment
   * @param[in] maxTriangleArea the maximum allowed triangle area
   */
  void setCompartmentMaxTriangleArea(std::size_t compartmentIndex,
                                     std::size_t maxTriangleArea);
  /**
   * @brief Get the maximum allowed triangle area for a given compartment
   *
   * @param[in] compartmentIndex the index of the compartment
   */
  [[nodiscard]] std::size_t
  getCompartmentMaxTriangleArea(std::size_t compartmentIndex) const;
  /**
   * @brief The maximum allowed triangle areas for each compartment in the mesh
   */
  [[nodiscard]] const std::vector<std::size_t> &
  getCompartmentMaxTriangleArea() const;
  /**
   * @brief The interior points for each compartment in the mesh
   *
   * Each interior point is chosen to be as far as possible from the edge of the
   * region, to reduce the chance that it ends up outside the meshed
   * approximation to the region as the boundary lines are simplified.
   *
   * @note There may be multiple interior points for a single compartment, for
   * example if that compartment has two disconnected regions
   */
  [[nodiscard]] const std::vector<std::vector<QPointF>> &
  getCompartmentInteriorPoints() const;
  /**
   * @brief The physical size and origin to use
   *
   * The boundary lines and mesh use pixel units internally, and are rescaled
   * to physical values using the supplied physical origin and pixel width.
   *
   * @param[in] pixelWidth the physical width of a pixel
   * @param[in] originPoint the physical location of the ``(0,0)`` pixel
   */
  void setPhysicalGeometry(double pixelWidth,
                           const QPointF &originPoint = QPointF(0, 0));
  /**
   * @brief The physical mesh vertices as a flat array of doubles
   *
   * For saving to the SBML document.
   */
  [[nodiscard]] std::vector<double> getVerticesAsFlatArray() const;
  /**
   * @brief The mesh triangle indices as a flat array of ints
   *
   * For saving to the SBML document.
   */
  [[nodiscard]] std::vector<int>
  getTriangleIndicesAsFlatArray(std::size_t compartmentIndex) const;
  /**
   * @brief The mesh triangle indices
   *
   * The indices of the triangles used in the mesh for each compartment
   */
  [[nodiscard]] const std::vector<std::vector<TriangulateTriangleIndex>> &
  getTriangleIndices() const;
  /**
   * @brief An image of the compartment boundary lines
   *
   * A pair of images are returned, the first is the image of the boundary
   * lines, and the second is a map from each pixel to the corresponding
   * boundary index, which can be used to identify which boundary was clicked on
   * by the user.
   *
   * @param[in] size the desired size of the image
   * @param[in] boldBoundaryIndex the boundary line to emphasize in the image
   */
  [[nodiscard]] std::pair<QImage, QImage>
  getBoundariesImages(const QSize &size, std::size_t boldBoundaryIndex) const;
  /**
   * @brief An image of the mesh
   *
   * A pair of images are returned, the first is the image of the mesh, and the
   * second is a map from each pixel to the corresponding compartment index,
   * which can be used to identify which compartment was clicked on by the user.
   *
   * @param[in] size the desired size of the image
   * @param[in] compartmentIndex the compartment to emphasize in the image
   * @returns a pair of images: mesh, compartment index
   */
  [[nodiscard]] std::pair<QImage, QImage>
  getMeshImages(const QSize &size, std::size_t compartmentIndex) const;
  /**
   * @brief The mesh in GMSH format
   *
   * @returns the mesh in GMSH format
   */
  [[nodiscard]] QString getGMSH() const;
};

} // namespace sme::mesh
