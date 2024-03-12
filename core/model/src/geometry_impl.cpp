#include "geometry_impl.hpp"
#include "sme/logger.hpp"
#include <QImage>
#include <QString>

namespace sme::geometry {

void fillMissingByDilation(std::vector<std::size_t> &arr, int nx, int ny,
                           int nz, std::size_t invalidIndex) {
  std::vector<std::size_t> arr_next{arr};
  const int maxIter{nx + ny + nz};
  std::size_t dx{1};
  std::size_t dy{static_cast<std::size_t>(nx)};
  std::size_t dz{dy * static_cast<std::size_t>(ny)};
  for (int iter = 0; iter < maxIter; ++iter) {
    bool finished{true};
    bool iteration_changed_something{false};
    for (int z = 0; z < nz; ++z) {
      for (int y = 0; y < ny; ++y) {
        for (int x = 0; x < nx; ++x) {
          auto i{(static_cast<std::size_t>(x) +
                  dy * static_cast<std::size_t>(y) +
                  dz * static_cast<std::size_t>(z))};
          if (arr[i] == invalidIndex) {
            // replace negative voxel with any valid face-/6-connected neighbour
            if (x > 0 && arr[i - dx] != invalidIndex) {
              arr_next[i] = arr[i - dx];
              iteration_changed_something = true;
            } else if (x + 1 < nx && arr[i + 1] != invalidIndex) {
              arr_next[i] = arr[i + dx];
              iteration_changed_something = true;
            } else if (y > 0 && arr[i - dy] != invalidIndex) {
              arr_next[i] = arr[i - dy];
              iteration_changed_something = true;
            } else if (y + 1 < ny && arr[i + dy] != invalidIndex) {
              arr_next[i] = arr[i + dy];
              iteration_changed_something = true;
            } else if (z > 0 && arr[i - dz] != invalidIndex) {
              arr_next[i] = arr[i - dz];
              iteration_changed_something = true;
            } else if (z + 1 < nz && arr[i + dz] != invalidIndex) {
              arr_next[i] = arr[i + dz];
              iteration_changed_something = true;
            } else {
              // voxel has no valid neighbour: need another iteration
              finished = false;
            }
          }
        }
      }
    }
    arr = arr_next;
    if (finished) {
      SPDLOG_DEBUG("Replaced all invalid indices after {} iterations", iter);
      return;
    }
    if (!iteration_changed_something) {
      SPDLOG_WARN("Last iteration {} did not modify any values - giving up",
                  iter);
      break;
    }
  }
  // set any remaining invalid indices to just point to the first element
  for (auto &a : arr) {
    if (a == invalidIndex) {
      SPDLOG_WARN("  - using 0 for invalid index");
      a = 0;
    }
  }
  SPDLOG_WARN("Failed to replace all invalid voxels");
}

#if SPDLOG_ACTIVE_LEVEL <= SPDLOG_LEVEL_TRACE
void saveDebuggingIndicesImageXY(const std::vector<std::size_t> &arrayPoints,
                                 int nx, int ny, int nz, std::size_t maxIndex,
                                 const QString &filename) {
  auto norm{static_cast<float>(maxIndex)};
  for (int z = 0; z < nz; ++z) {
    QImage img(nx, ny, QImage::Format_ARGB32_Premultiplied);
    img.fill(qRgba(0, 0, 0, 0));
    QColor c;
    for (int x = 0; x < nx; ++x) {
      for (int y = 0; y < ny; ++y) {
        auto i = arrayPoints[static_cast<std::size_t>(x + nx * y) +
                             static_cast<std::size_t>(nx * ny) * z];
        if (i <= maxIndex) {
          auto v{static_cast<float>(i) / norm};
          c.setHslF(1.0f - v, 1.0, 0.5f * v);
          img.setPixel(x, y, c.rgb());
        }
      }
    }
    img.save(filename + "XY_z" + QString::number(z) + ".png");
  }
}
#endif

} // namespace sme::geometry
