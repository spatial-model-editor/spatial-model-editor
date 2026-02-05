#pragma once

#include "sme/logger.hpp"
#include <QPoint>
#include <QSize>
#include <algorithm>

namespace sme::common {

struct Voxel {
  Voxel() = default;
  Voxel(QPoint vp, std::size_t vz) : p{vp}, z{vz} {}
  Voxel(int vx, int vy, std::size_t vz) : p{vx, vy}, z{vz} {}
  QPoint p{0, 0};
  std::size_t z{0};
  friend Voxel operator+(const Voxel &a, const Voxel &b) {
    return {a.p + b.p, a.z + b.z};
  }
  friend Voxel operator-(const Voxel &a, const Voxel &b) {
    return {a.p - b.p, a.z - b.z};
  }
  friend Voxel operator*(int a, const Voxel &b) {
    return {a * b.p, static_cast<std::size_t>(a) * b.z};
  }
  friend bool operator==(const Voxel &a, const Voxel &b) {
    return a.z == b.z && a.p == b.p;
  }
};

struct VoxelF {
  VoxelF() = default;
  VoxelF(double vx, double vy, double vz) : p{vx, vy}, z{vz} {}
  QPointF p{0, 0};
  double z{0};
};

struct Volume {
private:
  QSize xy{0, 0};
  std::size_t z{0};

public:
  Volume() = default;
  Volume(int vx, int vy, std::size_t vz) : xy{vx, vy}, z{vz} {}
  Volume(QSize vxy, std::size_t vz) : xy{vxy}, z{vz} {}
  [[nodiscard]] inline int width() const { return xy.width(); }
  [[nodiscard]] inline int height() const { return xy.height(); }
  [[nodiscard]] inline std::size_t depth() const { return z; }
  [[nodiscard]] inline std::size_t nVoxels() const {
    return static_cast<std::size_t>(xy.width() * xy.height()) * z;
  }
  friend bool operator==(const Volume &a, const Volume &b) {
    return a.width() == b.width() && a.height() == b.height() &&
           a.depth() == b.depth();
  }
};

class VolumeF {
private:
  QSizeF xy{0, 0};
  double z{0};

public:
  VolumeF() = default;
  VolumeF(double vx, double vy, double vz) : xy{vx, vy}, z{vz} {}
  VolumeF(QSizeF vxy, double vz) : xy{vxy}, z{vz} {}
  [[nodiscard]] inline double width() const { return xy.width(); }
  [[nodiscard]] inline double height() const { return xy.height(); }
  [[nodiscard]] inline double depth() const { return z; }
  [[nodiscard]] inline double volume() const {
    return xy.width() * xy.height() * z;
  }
  friend VolumeF operator*(double a, const VolumeF &b) {
    return {a * b.width(), a * b.height(), a * b.depth()};
  }
  friend VolumeF operator*(const VolumeF &a, double b) { return b * a; }
  friend VolumeF operator*(const VolumeF &a, const Volume &b) {
    return {a.width() * static_cast<double>(b.width()),
            a.height() * static_cast<double>(b.height()),
            a.depth() * static_cast<double>(b.depth())};
  }
  friend VolumeF operator*(const Volume &a, const VolumeF &b) { return b * a; }
  friend VolumeF operator/(const VolumeF &a, const Volume &b) {
    return {a.width() / static_cast<double>(b.width()),
            a.height() / static_cast<double>(b.height()),
            a.depth() / static_cast<double>(b.depth())};
  }
};

[[nodiscard]] inline int toVoxelIndex(double coordinate, double origin,
                                      double voxelSize, int nVoxels) noexcept {
  if (nVoxels <= 1 || voxelSize <= 0.0) {
    return 0;
  }
  return std::clamp(static_cast<int>((coordinate - origin) / voxelSize), 0,
                    nVoxels - 1);
}

[[nodiscard]] inline int yIndex(int y, int height, bool invertY) noexcept {
  return invertY ? (height - 1 - y) : y;
}

[[nodiscard]] inline std::size_t
voxelArrayIndex(const Volume &volume, int x, int y, std::size_t z,
                bool invertY = false) noexcept {
  return static_cast<std::size_t>(x + volume.width() *
                                          yIndex(y, volume.height(), invertY)) +
         static_cast<std::size_t>(volume.width() * volume.height()) * z;
}

[[nodiscard]] inline std::size_t
voxelArrayIndex(const Volume &volume, const Voxel &voxel,
                bool invertY = false) noexcept {
  return voxelArrayIndex(volume, voxel.p.x(), voxel.p.y(), voxel.z, invertY);
}

} // namespace sme::common
