//  - QPointIndexer class:
//     - given vector of QPoints
//     - lookup index of any QPoint in the vector
//     - returns std::optional with index if found
//  - QPointUniqueIndexer class:
//     - as above but removes duplicated QPoints first

#pragma once

#include "sme/logger.hpp"
#include "sme/voxel.hpp"
#include <QImage>
#include <QPoint>
#include <QRgb>
#include <cstddef>
#include <string>
#include <utility>
#include <vector>

namespace sme::geometry {

/**
 * @brief Convert a (2d) QPoint to an integer
 *
 * Given a bounding box and a QPoint within this box, return an integer
 * that identifies this point.
 */
class QPointFlattener {
private:
  QSize box;

public:
  explicit QPointFlattener(const QSize &boundingBox);
  /**
   * @brief Check if the point is inside the bounding box
   */
  [[nodiscard]] bool isValid(const QPoint &point) const;
  /**
   * @brief Return an index corresponding to the given point
   */
  [[nodiscard]] std::size_t flatten(const QPoint &point) const;
};

/**
 * @brief Fast look-up of points within a 2d bounding box
 *
 * Stores a set of points that lie within a bounding box. Allows fast look-up of
 * the index of each point.
 */
class QPointIndexer {
private:
  QPointFlattener flattener;
  std::size_t nPoints = 0;
  std::vector<std::size_t> pointIndex;

public:
  explicit QPointIndexer(const QSize &boundingBox,
                         const std::vector<QPoint> &qPoints = {});
  void addPoints(const std::vector<QPoint> &qPoints);
  [[nodiscard]] std::optional<std::size_t> getIndex(const QPoint &point) const;
  [[nodiscard]] std::size_t size() const;
};

/**
 * @brief Fast look-up of unique points within a bounding box
 *
 * Stores a set of unique points that lie within a bounding box. Allows fast
 * look-up of the index of each point.
 * @see QPointIndexer
 */
class QPointUniqueIndexer {
private:
  QPointFlattener flattener;
  std::size_t nPoints = 0;
  std::vector<std::size_t> pointIndex;
  std::vector<QPoint> points;

public:
  explicit QPointUniqueIndexer(const QSize &boundingBox,
                               const std::vector<QPoint> &qPoints = {});
  void addPoints(const std::vector<QPoint> &qPoints);
  [[nodiscard]] std::optional<std::size_t> getIndex(const QPoint &point) const;
  [[nodiscard]] std::vector<QPoint> getPoints() const;
};

/**
 * @brief Convert a (3d) Voxel to an integer
 *
 * Given a bounding box and a Voxel within this box, return an integer
 * that identifies this voxel.
 */
class VoxelFlattener {
private:
  int nx;
  int ny;
  int nz;

public:
  explicit VoxelFlattener(const common::Volume &vSize);
  explicit VoxelFlattener(int nx, int ny, int nz);
  /**
   * @brief Check if the point is inside the bounding box
   */
  [[nodiscard]] bool isValid(const common::Voxel &voxel) const;
  /**
   * @brief Return an index corresponding to the given point
   */
  [[nodiscard]] std::size_t flatten(const common::Voxel &voxel) const;
};

/**
 * @brief Fast look-up of voxels within a 3d bounding box
 *
 * Stores a set of voxels that lie within a bounding box. Allows fast look-up of
 * the index of each voxel.
 */
class VoxelIndexer {
private:
  VoxelFlattener flattener;
  std::size_t nVoxels{0};
  std::vector<std::size_t> voxelIndex;

public:
  explicit VoxelIndexer(const common::Volume &vSize,
                        const std::vector<common::Voxel> &voxels = {});
  explicit VoxelIndexer(int nx, int ny, int nz,
                        const std::vector<common::Voxel> &voxels = {});
  void addVoxels(const std::vector<common::Voxel> &voxels);
  [[nodiscard]] std::optional<std::size_t>
  getIndex(const common::Voxel &voxel) const;
  [[nodiscard]] std::size_t size() const;
};

} // namespace sme::geometry
