#pragma once

#include "sme/logger.hpp"
#include <QPoint>
#include <QSize>

namespace sme::common {

struct Voxel {
  Voxel() = default;
  Voxel(QPoint p, std::size_t z) : p{p}, z{z} {}
  Voxel(int x, int y, std::size_t z) : p{x, y}, z{z} {}
  QPoint p{0, 0};
  std::size_t z{0};
};

inline Voxel operator+(const Voxel &a, const Voxel &b) {
  return {a.p + b.p, a.z + b.z};
}
inline Voxel operator-(const Voxel &a, const Voxel &b) {
  return {a.p - b.p, a.z - b.z};
}
inline Voxel operator*(int a, const Voxel &b) { return {a * b.p, a * b.z}; }
inline bool operator==(const Voxel &a, const Voxel &b) {
  return a.z == b.z && a.p == b.p;
}

struct VoxelF {
  VoxelF() = default;
  VoxelF(double x, double y, double z) : p{x, y}, z{z} {}
  QPointF p{0, 0};
  double z{0};
};

struct Volume {
private:
  QSize xy{0, 0};
  std::size_t z{0};

public:
  Volume() = default;
  Volume(int x, int y, std::size_t z) : xy{x, y}, z{z} {}
  Volume(QSize xy, std::size_t z) : xy{xy}, z{z} {}
  [[nodiscard]] inline int width() const { return xy.width(); }
  [[nodiscard]] inline int height() const { return xy.height(); }
  [[nodiscard]] inline std::size_t depth() const { return z; }
  [[nodiscard]] inline std::size_t nVoxels() const {
    return static_cast<std::size_t>(xy.width() * xy.height()) * z;
  }
};

inline bool operator==(const Volume &a, const Volume &b) {
  return a.width() == b.width() && a.height() == b.height() &&
         a.depth() == b.depth();
}
inline bool operator!=(const Volume &a, const Volume &b) {
  return a.width() != b.width() || a.height() != b.height() ||
         a.depth() != b.depth();
}

class VolumeF {
private:
  QSizeF xy{0, 0};
  double z{0};

public:
  VolumeF() = default;
  VolumeF(double x, double y, double z) : xy{x, y}, z{z} {}
  VolumeF(QSizeF xy, double z) : xy{xy}, z{z} {}
  [[nodiscard]] inline double width() const { return xy.width(); }
  [[nodiscard]] inline double height() const { return xy.height(); }
  [[nodiscard]] inline double depth() const { return z; }
  [[nodiscard]] inline double volume() const {
    return xy.width() * xy.height() * z;
  }
};

inline VolumeF operator*(double a, const VolumeF &b) {
  return {a * b.width(), a * b.height(), a * b.depth()};
}

inline VolumeF operator*(const VolumeF &a, double b) { return b * a; }

inline VolumeF operator*(const VolumeF &a, const Volume &b) {
  return {a.width() * static_cast<double>(b.width()),
          a.height() * static_cast<double>(b.height()),
          a.depth() * static_cast<double>(b.depth())};
}

inline VolumeF operator*(const Volume &a, const VolumeF &b) { return b * a; }

inline VolumeF operator/(const VolumeF &a, const Volume &b) {
  return {a.width() / static_cast<double>(b.width()),
          a.height() / static_cast<double>(b.height()),
          a.depth() / static_cast<double>(b.depth())};
}

} // namespace sme::common
