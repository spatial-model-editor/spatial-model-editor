#pragma once

#include "sme/logger.hpp"
#include <QPoint>
#include <QSize>
#include <algorithm>

namespace sme::common {

/**
 * @brief Integer voxel coordinate ``(x, y, z)``.
 */
struct Voxel {
  /**
   * @brief Construct a voxel at the origin.
   */
  Voxel() = default;
  /**
   * @brief Construct from 2D point and z index.
   */
  Voxel(QPoint vp, std::size_t vz) : p{vp}, z{vz} {}
  /**
   * @brief Construct from explicit coordinates.
   */
  Voxel(int vx, int vy, std::size_t vz) : p{vx, vy}, z{vz} {}
  /**
   * @brief Integer ``(x, y)`` pixel coordinate.
   */
  QPoint p{0, 0};
  /**
   * @brief Integer z index.
   */
  std::size_t z{0};
  /**
   * @brief Component-wise voxel addition.
   */
  friend Voxel operator+(const Voxel &a, const Voxel &b) {
    return {a.p + b.p, a.z + b.z};
  }
  /**
   * @brief Component-wise voxel subtraction.
   */
  friend Voxel operator-(const Voxel &a, const Voxel &b) {
    return {a.p - b.p, a.z - b.z};
  }
  /**
   * @brief Scalar multiplication by integer.
   */
  friend Voxel operator*(int a, const Voxel &b) {
    return {a * b.p, static_cast<std::size_t>(a) * b.z};
  }
  /**
   * @brief Equality comparison.
   */
  friend bool operator==(const Voxel &a, const Voxel &b) {
    return a.z == b.z && a.p == b.p;
  }
};

/**
 * @brief Floating-point voxel coordinate ``(x, y, z)``.
 */
struct VoxelF {
  /**
   * @brief Construct a voxel at the origin.
   */
  VoxelF() = default;
  /**
   * @brief Construct from explicit coordinates.
   */
  VoxelF(double vx, double vy, double vz) : p{vx, vy}, z{vz} {}
  /**
   * @brief Floating-point ``(x, y)`` coordinate.
   */
  QPointF p{0, 0};
  /**
   * @brief Floating-point z coordinate.
   */
  double z{0};
};

/**
 * @brief Integer volume dimensions ``(width, height, depth)``.
 */
struct Volume {
private:
  QSize xy{0, 0};
  std::size_t z{0};

public:
  /**
   * @brief Construct an empty volume.
   */
  Volume() = default;
  /**
   * @brief Construct from explicit dimensions.
   */
  Volume(int vx, int vy, std::size_t vz) : xy{vx, vy}, z{vz} {}
  /**
   * @brief Construct from 2D size and depth.
   */
  Volume(QSize vxy, std::size_t vz) : xy{vxy}, z{vz} {}
  /**
   * @brief Width in voxels.
   */
  [[nodiscard]] inline int width() const { return xy.width(); }
  /**
   * @brief Height in voxels.
   */
  [[nodiscard]] inline int height() const { return xy.height(); }
  /**
   * @brief Depth in voxels.
   */
  [[nodiscard]] inline std::size_t depth() const { return z; }
  /**
   * @brief Total voxel count.
   */
  [[nodiscard]] inline std::size_t nVoxels() const {
    return static_cast<std::size_t>(xy.width() * xy.height()) * z;
  }
  /**
   * @brief Equality comparison.
   */
  friend bool operator==(const Volume &a, const Volume &b) {
    return a.width() == b.width() && a.height() == b.height() &&
           a.depth() == b.depth();
  }
};

/**
 * @brief Floating-point volume dimensions ``(width, height, depth)``.
 */
class VolumeF {
private:
  QSizeF xy{0, 0};
  double z{0};

public:
  /**
   * @brief Construct an empty volume.
   */
  VolumeF() = default;
  /**
   * @brief Construct from explicit dimensions.
   */
  VolumeF(double vx, double vy, double vz) : xy{vx, vy}, z{vz} {}
  /**
   * @brief Construct from 2D size and depth.
   */
  VolumeF(QSizeF vxy, double vz) : xy{vxy}, z{vz} {}
  /**
   * @brief Width.
   */
  [[nodiscard]] inline double width() const { return xy.width(); }
  /**
   * @brief Height.
   */
  [[nodiscard]] inline double height() const { return xy.height(); }
  /**
   * @brief Depth.
   */
  [[nodiscard]] inline double depth() const { return z; }
  /**
   * @brief 3D volume (width * height * depth).
   */
  [[nodiscard]] inline double volume() const {
    return xy.width() * xy.height() * z;
  }
  /**
   * @brief Scalar multiplication.
   */
  friend VolumeF operator*(double a, const VolumeF &b) {
    return {a * b.width(), a * b.height(), a * b.depth()};
  }
  /**
   * @brief Scalar multiplication.
   */
  friend VolumeF operator*(const VolumeF &a, double b) { return b * a; }
  /**
   * @brief Element-wise multiplication by integer dimensions.
   */
  friend VolumeF operator*(const VolumeF &a, const Volume &b) {
    return {a.width() * static_cast<double>(b.width()),
            a.height() * static_cast<double>(b.height()),
            a.depth() * static_cast<double>(b.depth())};
  }
  /**
   * @brief Element-wise multiplication by integer dimensions.
   */
  friend VolumeF operator*(const Volume &a, const VolumeF &b) { return b * a; }
  /**
   * @brief Element-wise division by integer dimensions.
   */
  friend VolumeF operator/(const VolumeF &a, const Volume &b) {
    return {a.width() / static_cast<double>(b.width()),
            a.height() / static_cast<double>(b.height()),
            a.depth() / static_cast<double>(b.depth())};
  }
};

/**
 * @brief Convert a physical coordinate into a clamped voxel index.
 */
[[nodiscard]] inline int toVoxelIndex(double coordinate, double origin,
                                      double voxelSize, int nVoxels) noexcept {
  if (nVoxels <= 1 || voxelSize <= 0.0) {
    return 0;
  }
  return std::clamp(static_cast<int>((coordinate - origin) / voxelSize), 0,
                    nVoxels - 1);
}

/**
 * @brief Return y index with optional top/bottom inversion.
 */
[[nodiscard]] inline int yIndex(int y, int height, bool invertY) noexcept {
  return invertY ? (height - 1 - y) : y;
}

/**
 * @brief Flatten voxel coordinate into a linear index.
 */
[[nodiscard]] inline std::size_t
voxelArrayIndex(const Volume &volume, int x, int y, std::size_t z,
                bool invertY = false) noexcept {
  return static_cast<std::size_t>(x + volume.width() *
                                          yIndex(y, volume.height(), invertY)) +
         static_cast<std::size_t>(volume.width() * volume.height()) * z;
}

/**
 * @brief Flatten voxel coordinate into a linear index.
 */
[[nodiscard]] inline std::size_t
voxelArrayIndex(const Volume &volume, const Voxel &voxel,
                bool invertY = false) noexcept {
  return voxelArrayIndex(volume, voxel.p.x(), voxel.p.y(), voxel.z, invertY);
}

} // namespace sme::common
