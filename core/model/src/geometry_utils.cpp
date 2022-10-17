#include "sme/geometry_utils.hpp"
#include <algorithm>
#include <initializer_list>
#include <limits>
#include <optional>
#include <stdexcept>

namespace sme::geometry {

constexpr std::size_t NULL_INDEX{std::numeric_limits<std::size_t>::max()};

QPointFlattener::QPointFlattener(const QSize &boundingBox) : box(boundingBox) {}

bool QPointFlattener::isValid(const QPoint &point) const {
  bool xInside = (point.x() >= 0) && (point.x() < box.width());
  bool yInside = (point.y() >= 0) && (point.y() < box.height());
  return xInside && yInside;
}

std::size_t QPointFlattener::flatten(const QPoint &point) const {
  return static_cast<std::size_t>(point.x() * box.height() + point.y());
}

QPointIndexer::QPointIndexer(const QSize &boundingBox,
                             const std::vector<QPoint> &qPoints)
    : flattener(boundingBox),
      pointIndex(
          static_cast<std::size_t>(boundingBox.width() * boundingBox.height()),
          NULL_INDEX) {
  addPoints(qPoints);
}

void QPointIndexer::addPoints(const std::vector<QPoint> &qPoints) {
  for (const auto &point : qPoints) {
    if (!flattener.isValid(point)) {
      throw std::invalid_argument("invalid point: not within bounding box");
    }
    pointIndex[flattener.flatten(point)] = nPoints++;
  }
}

std::optional<std::size_t> QPointIndexer::getIndex(const QPoint &point) const {
  if (!flattener.isValid(point)) {
    return {};
  }
  auto index = pointIndex[flattener.flatten(point)];
  if (index == NULL_INDEX) {
    return {};
  }
  return index;
}

std::size_t QPointIndexer::size() const { return nPoints; }

QPointUniqueIndexer::QPointUniqueIndexer(const QSize &boundingBox,
                                         const std::vector<QPoint> &qPoints)
    : flattener(boundingBox),
      pointIndex(
          static_cast<std::size_t>(boundingBox.width() * boundingBox.height()),
          NULL_INDEX) {
  addPoints(qPoints);
}

void QPointUniqueIndexer::addPoints(const std::vector<QPoint> &qPoints) {
  // add only unique points to vector & index
  for (const auto &point : qPoints) {
    if (!flattener.isValid(point)) {
      throw std::invalid_argument("invalid point: not within bounding box");
    }
    auto existingIndex = pointIndex[flattener.flatten(point)];
    if (existingIndex == NULL_INDEX) {
      pointIndex[flattener.flatten(point)] = nPoints++;
      points.push_back(point);
    }
  }
}

std::optional<std::size_t>
QPointUniqueIndexer::getIndex(const QPoint &point) const {
  if (!flattener.isValid(point)) {
    return {};
  }
  auto index = pointIndex[flattener.flatten(point)];
  if (index == NULL_INDEX) {
    return {};
  }
  return index;
}

std::vector<QPoint> QPointUniqueIndexer::getPoints() const { return points; }

VoxelFlattener::VoxelFlattener(const VSize &vSize)
    : nx{vSize.width()}, ny{vSize.height()}, nz{static_cast<int>(
                                                 vSize.depth())} {}

VoxelFlattener::VoxelFlattener(int nx, int ny, int nz)
    : nx{nx}, ny{ny}, nz{nz} {}

bool VoxelFlattener::isValid(const Voxel &voxel) const {
  bool xInside = (voxel.p.x() >= 0) && (voxel.p.x() < nx);
  bool yInside = (voxel.p.y() >= 0) && (voxel.p.y() < ny);
  bool zInside = (voxel.z >= 0) && (voxel.z < nz);
  return xInside && yInside && zInside;
}

std::size_t VoxelFlattener::flatten(const Voxel &voxel) const {
  return static_cast<std::size_t>(voxel.p.x() + voxel.p.y() * nx +
                                  voxel.z * nx * ny);
}

VoxelIndexer::VoxelIndexer(const VSize &vSize, const std::vector<Voxel> &voxels)
    : flattener(vSize), voxelIndex(vSize.nVoxels(), NULL_INDEX) {
  addVoxels(voxels);
}

VoxelIndexer::VoxelIndexer(int nx, int ny, int nz,
                           const std::vector<Voxel> &voxels)
    : flattener(nx, ny, nz),
      voxelIndex(static_cast<std::size_t>(nx * ny * nz), NULL_INDEX) {
  addVoxels(voxels);
}

void VoxelIndexer::addVoxels(const std::vector<Voxel> &voxels) {
  for (const auto &voxel : voxels) {
    if (!flattener.isValid(voxel)) {
      throw std::invalid_argument("invalid voxel: not within bounding box");
    }
    voxelIndex[flattener.flatten(voxel)] = nVoxels++;
  }
}

std::optional<std::size_t> VoxelIndexer::getIndex(const Voxel &voxel) const {
  if (!flattener.isValid(voxel)) {
    return {};
  }
  auto index = voxelIndex[flattener.flatten(voxel)];
  if (index == NULL_INDEX) {
    return {};
  }
  return index;
}

std::size_t VoxelIndexer::size() const { return nVoxels; }

} // namespace sme::geometry
