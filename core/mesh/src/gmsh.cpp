#include "sme/gmsh.hpp"
#include "sme/logger.hpp"
#include "sme/utils.hpp"
#include <QStringList>
#include <algorithm>
#include <array>
#include <cmath>
#include <cstddef>
#include <cstdint>
#include <deque>
#include <dune/common/exceptions.hh>
#include <dune/grid/common/mcmgmapper.hh>
#include <dune/grid/common/rangegenerators.hh>
#include <dune/grid/io/file/gmshreader.hh>
#include <dune/grid/uggrid.hh>
#include <limits>
#include <map>
#include <optional>
#include <string>
#include <vector>

namespace sme::mesh {

namespace {

struct Point3d {
  double x{};
  double y{};
  double z{};
};

[[nodiscard]] Point3d toPoint3d(const common::VoxelF &v) {
  return {v.p.x(), v.p.y(), v.z};
}

[[nodiscard]] std::optional<std::array<double, 9>>
inverse3x3(const std::array<double, 9> &m) {
  const double det = m[0] * (m[4] * m[8] - m[5] * m[7]) -
                     m[1] * (m[3] * m[8] - m[5] * m[6]) +
                     m[2] * (m[3] * m[7] - m[4] * m[6]);
  constexpr double minAbsDet{1e-18};
  if (std::abs(det) < minAbsDet) {
    return std::nullopt;
  }
  const double invDet = 1.0 / det;
  std::array<double, 9> inv{};
  inv[0] = invDet * (m[4] * m[8] - m[5] * m[7]);
  inv[1] = invDet * (m[2] * m[7] - m[1] * m[8]);
  inv[2] = invDet * (m[1] * m[5] - m[2] * m[4]);
  inv[3] = invDet * (m[5] * m[6] - m[3] * m[8]);
  inv[4] = invDet * (m[0] * m[8] - m[2] * m[6]);
  inv[5] = invDet * (m[2] * m[3] - m[0] * m[5]);
  inv[6] = invDet * (m[3] * m[7] - m[4] * m[6]);
  inv[7] = invDet * (m[1] * m[6] - m[0] * m[7]);
  inv[8] = invDet * (m[0] * m[4] - m[1] * m[3]);
  return inv;
}

[[nodiscard]] bool pointInsideTetrahedron(const Point3d &p,
                                          const std::array<Point3d, 4> &v,
                                          const std::array<double, 9> &invM) {
  const Point3d r{p.x - v[0].x, p.y - v[0].y, p.z - v[0].z};
  const double l1 = invM[0] * r.x + invM[1] * r.y + invM[2] * r.z;
  const double l2 = invM[3] * r.x + invM[4] * r.y + invM[5] * r.z;
  const double l3 = invM[6] * r.x + invM[7] * r.y + invM[8] * r.z;
  const double l0 = 1.0 - l1 - l2 - l3;
  constexpr double tol = 1e-10;
  return l0 >= -tol && l1 >= -tol && l2 >= -tol && l3 >= -tol &&
         l0 <= 1.0 + tol && l1 <= 1.0 + tol && l2 <= 1.0 + tol &&
         l3 <= 1.0 + tol;
}

[[nodiscard]] int getImageDimension(double extent, double maxExtent,
                                    int maxVoxelsPerDimension) {
  if (maxVoxelsPerDimension <= 1 || maxExtent <= 0.0) {
    return 1;
  }
  return std::clamp(
      static_cast<int>(maxVoxelsPerDimension * extent / maxExtent), 1,
      maxVoxelsPerDimension);
}

[[nodiscard]] std::pair<int, int> voxelIndexRange(double minCoord,
                                                  double maxCoord,
                                                  double lowerBound,
                                                  double extent, int nVoxels) {
  if (nVoxels <= 1 || extent <= 0.0) {
    return {0, 0};
  }
  const double scale = static_cast<double>(nVoxels) / extent;
  const double lower = scale * (minCoord - lowerBound) - 0.5;
  const double upper = scale * (maxCoord - lowerBound) - 0.5;
  const int iMin =
      std::clamp(static_cast<int>(std::ceil(lower)), 0, nVoxels - 1);
  const int iMax =
      std::clamp(static_cast<int>(std::floor(upper)), 0, nVoxels - 1);
  return {iMin, iMax};
}

[[nodiscard]] double voxelCenterCoordinate(int index, double lowerBound,
                                           double extent, int nVoxels) {
  if (nVoxels <= 1 || extent <= 0.0) {
    return lowerBound;
  }
  return lowerBound + extent * (0.5 + static_cast<double>(index)) /
                          static_cast<double>(nVoxels);
}

[[nodiscard]] double voxelSize(double extent, int nVoxels) {
  if (extent <= 0.0 || nVoxels <= 0) {
    return 1.0;
  }
  return extent / static_cast<double>(nVoxels);
}

[[nodiscard]] std::size_t flatIndex(int x, int y, int z, int nx, int ny) {
  return static_cast<std::size_t>(x + nx * (y + ny * z));
}

void fillNullVoxelsWithNearestColor(common::ImageStack &images,
                                    QRgb nullColor) {
  const auto vol = images.volume();
  const int nx = vol.width();
  const int ny = vol.height();
  const int nz = static_cast<int>(vol.depth());
  if (nx <= 0 || ny <= 0 || nz <= 0) {
    return;
  }

  struct VoxelIndex {
    int x{};
    int y{};
    int z{};
  };

  std::deque<VoxelIndex> queue;
  std::vector<uint8_t> visited(vol.nVoxels(), 0);
  std::vector<QRgb> nearestColor(vol.nVoxels(), nullColor);

  for (int z = 0; z < nz; ++z) {
    for (int y = 0; y < ny; ++y) {
      for (int x = 0; x < nx; ++x) {
        const auto color = images[static_cast<std::size_t>(z)].pixel(x, y);
        if (color == nullColor) {
          continue;
        }
        const auto i = flatIndex(x, y, z, nx, ny);
        visited[i] = 1;
        nearestColor[i] = color;
        queue.push_back({x, y, z});
      }
    }
  }
  if (queue.empty()) {
    return;
  }

  constexpr std::array<VoxelIndex, 6> offsets{
      VoxelIndex{-1, 0, 0}, VoxelIndex{1, 0, 0},  VoxelIndex{0, -1, 0},
      VoxelIndex{0, 1, 0},  VoxelIndex{0, 0, -1}, VoxelIndex{0, 0, 1}};

  while (!queue.empty()) {
    const auto voxel = queue.front();
    queue.pop_front();
    const auto i = flatIndex(voxel.x, voxel.y, voxel.z, nx, ny);
    const auto color = nearestColor[i];
    for (const auto &offset : offsets) {
      const int x = voxel.x + offset.x;
      const int y = voxel.y + offset.y;
      const int z = voxel.z + offset.z;
      if (x < 0 || x >= nx || y < 0 || y >= ny || z < 0 || z >= nz) {
        continue;
      }
      const auto j = flatIndex(x, y, z, nx, ny);
      if (visited[j] != 0) {
        continue;
      }
      visited[j] = 1;
      nearestColor[j] = color;
      queue.push_back({x, y, z});
    }
  }

  for (int z = 0; z < nz; ++z) {
    for (int y = 0; y < ny; ++y) {
      for (int x = 0; x < nx; ++x) {
        auto &img = images[static_cast<std::size_t>(z)];
        if (img.pixel(x, y) != nullColor) {
          continue;
        }
        const auto i = flatIndex(x, y, z, nx, ny);
        img.setPixel(x, y, nearestColor[i]);
      }
    }
  }
}

void fillRemainingNullVoxels(common::ImageStack &images, QRgb nullColor,
                             QRgb fallbackColor) {
  const auto vol = images.volume();
  for (std::size_t z = 0; z < vol.depth(); ++z) {
    for (int y = 0; y < vol.height(); ++y) {
      for (int x = 0; x < vol.width(); ++x) {
        auto &img = images[z];
        if (img.pixel(x, y) == nullColor) {
          img.setPixel(x, y, fallbackColor);
        }
      }
    }
  }
}

[[nodiscard]] bool pointInsideTriangle2d(double x, double y,
                                         const std::array<Point3d, 3> &v) {
  const double det = (v[1].y - v[2].y) * (v[0].x - v[2].x) +
                     (v[2].x - v[1].x) * (v[0].y - v[2].y);
  constexpr double minAbsDet{1e-18};
  if (std::abs(det) < minAbsDet) {
    return false;
  }
  const double l1 =
      ((v[1].y - v[2].y) * (x - v[2].x) + (v[2].x - v[1].x) * (y - v[2].y)) /
      det;
  const double l2 =
      ((v[2].y - v[0].y) * (x - v[2].x) + (v[0].x - v[2].x) * (y - v[2].y)) /
      det;
  const double l3 = 1.0 - l1 - l2;
  constexpr double tol = 1e-10;
  return l1 >= -tol && l2 >= -tol && l3 >= -tol && l1 <= 1.0 + tol &&
         l2 <= 1.0 + tol && l3 <= 1.0 + tol;
}

struct GMSHReadResult {
  std::optional<GMSHMesh> mesh{};
  std::string error{};
};

template <int dim>
[[nodiscard]] GMSHReadResult readGMSHMeshDim(const QString &filename) {
  static_assert(dim == 2 || dim == 3);
  using Grid = Dune::UGGrid<dim>;
  constexpr const char *dimName = dim == 3 ? "3d" : "2d";
  constexpr int simplexCorners = dim == 3 ? 4 : 3;
  // dune assumes C locale for reading gmsh files
  common::ScopedCLocale scopedCLocale;
  std::vector<int> boundaryToPhysicalEntity;
  std::vector<int> elementToPhysicalEntity;
  std::unique_ptr<Grid> grid;
  try {
    grid = Dune::GmshReader<Grid>::read(filename.toStdString(),
                                        boundaryToPhysicalEntity,
                                        elementToPhysicalEntity,
                                        false, // verbose
                                        false  // insertBoundarySegments
    );
  } catch (const Dune::Exception &e) {
    return {.mesh = std::nullopt, .error = e.what()};
  }
  if (grid == nullptr || elementToPhysicalEntity.empty()) {
    return {.mesh = std::nullopt,
            .error = std::string("no ") + dimName + " elements"};
  }

  auto gridView = grid->leafGridView();
  if (gridView.size(0) == 0) {
    return {.mesh = std::nullopt,
            .error = std::string("empty ") + dimName + " grid"};
  }

  using GridView = decltype(gridView);
  Dune::MultipleCodimMultipleGeomTypeMapper<GridView> vertexMapper(
      gridView, Dune::mcmgVertexLayout());
  Dune::MultipleCodimMultipleGeomTypeMapper<GridView> elementMapper(
      gridView, Dune::mcmgElementLayout());

  GMSHMesh mesh;
  mesh.vertices.resize(vertexMapper.size());
  for (const auto &vertex : vertices(gridView)) {
    auto i = vertexMapper.index(vertex);
    auto p = vertex.geometry().corner(0);
    if constexpr (dim == 3) {
      mesh.vertices[i] = {p[0], p[1], p[2]};
    } else {
      mesh.vertices[i] = {p[0], p[1], 0.0};
    }
  }

  if constexpr (dim == 3) {
    mesh.tetrahedra.reserve(gridView.size(0));
  } else {
    mesh.triangles.reserve(gridView.size(0));
  }
  for (const auto &element : elements(gridView)) {
    const auto iElement =
        static_cast<std::size_t>(elementMapper.index(element));
    if (iElement >= elementToPhysicalEntity.size()) {
      return {.mesh = std::nullopt,
              .error = std::string(dimName) + " element index mismatch"};
    }
    const auto &geo = element.geometry();
    if (!(geo.type().isSimplex() && geo.corners() == simplexCorners)) {
      continue;
    }
    if constexpr (dim == 3) {
      GMSHTetrahedron tet;
      tet.physicalTag = elementToPhysicalEntity[iElement];
      for (int i = 0; i < 4; ++i) {
        tet.vertexIndices[static_cast<std::size_t>(i)] =
            vertexMapper.subIndex(element, i, dim);
      }
      mesh.tetrahedra.push_back(tet);
    } else {
      GMSHTriangle tri;
      tri.physicalTag = elementToPhysicalEntity[iElement];
      for (int i = 0; i < 3; ++i) {
        tri.vertexIndices[static_cast<std::size_t>(i)] =
            vertexMapper.subIndex(element, i, dim);
      }
      mesh.triangles.push_back(tri);
    }
  }

  const bool hasNoElements =
      dim == 3 ? mesh.tetrahedra.empty() : mesh.triangles.empty();
  if (mesh.vertices.empty() || hasNoElements) {
    return {.mesh = std::nullopt,
            .error =
                std::string("no ") + (dim == 3 ? "tetrahedra" : "triangles")};
  }
  return {.mesh = mesh, .error = {}};
}

[[nodiscard]] std::vector<int> getSortedPhysicalTags(const GMSHMesh &mesh) {
  std::vector<int> sortedTags;
  sortedTags.reserve(mesh.tetrahedra.size() + mesh.triangles.size());
  for (const auto &tet : mesh.tetrahedra) {
    if (std::ranges::find(sortedTags, tet.physicalTag) == sortedTags.cend()) {
      sortedTags.push_back(tet.physicalTag);
    }
  }
  for (const auto &tri : mesh.triangles) {
    if (std::ranges::find(sortedTags, tri.physicalTag) == sortedTags.cend()) {
      sortedTags.push_back(tri.physicalTag);
    }
  }
  std::ranges::sort(sortedTags);
  return sortedTags;
}

} // namespace

std::optional<GMSHMesh> readGMSHMesh(const QString &filename) {
  common::ScopedCLocale scopedCLocale;
  const auto read3d = readGMSHMeshDim<3>(filename);
  if (read3d.mesh.has_value()) {
    return read3d.mesh;
  }
  const auto read2d = readGMSHMeshDim<2>(filename);
  if (read2d.mesh.has_value()) {
    return read2d.mesh;
  }
  QStringList details;
  if (!read3d.error.empty()) {
    details.push_back(QString("3d: %1").arg(read3d.error.c_str()));
  }
  if (!read2d.error.empty()) {
    details.push_back(QString("2d: %1").arg(read2d.error.c_str()));
  }
  SPDLOG_ERROR("Failed to parse gmsh file '{}'", filename.toStdString());
  if (!details.empty()) {
    SPDLOG_ERROR("{}", details.join("; ").toStdString());
  }
  return std::nullopt;
}

common::ImageStack voxelizeGMSHMesh(const GMSHMesh &mesh,
                                    int maxVoxelsPerDimension,
                                    bool includeBackground) {
  if (mesh.vertices.empty() ||
      (mesh.tetrahedra.empty() && mesh.triangles.empty())) {
    return {};
  }

  const auto compartmentTags = getSortedPhysicalTags(mesh);
  if (compartmentTags.empty()) {
    return {};
  }

  std::map<int, QRgb> tagToColor;
  for (std::size_t i = 0; i < compartmentTags.size(); ++i) {
    tagToColor[compartmentTags[i]] = common::indexedColors()[i].rgb();
  }

  Point3d meshMin{std::numeric_limits<double>::max(),
                  std::numeric_limits<double>::max(),
                  std::numeric_limits<double>::max()};
  Point3d meshMax{std::numeric_limits<double>::lowest(),
                  std::numeric_limits<double>::lowest(),
                  std::numeric_limits<double>::lowest()};
  for (const auto &v : mesh.vertices) {
    auto p = toPoint3d(v);
    meshMin.x = std::min(meshMin.x, p.x);
    meshMin.y = std::min(meshMin.y, p.y);
    meshMin.z = std::min(meshMin.z, p.z);
    meshMax.x = std::max(meshMax.x, p.x);
    meshMax.y = std::max(meshMax.y, p.y);
    meshMax.z = std::max(meshMax.z, p.z);
  }

  const double width = std::max(0.0, meshMax.x - meshMin.x);
  const double height = std::max(0.0, meshMax.y - meshMin.y);
  const double depth = std::max(0.0, meshMax.z - meshMin.z);
  const double maxExtent = mesh.tetrahedra.empty()
                               ? std::max(width, height)
                               : std::max({width, height, depth});
  const int nMax = std::max(maxVoxelsPerDimension, 1);
  const int nx = getImageDimension(width, maxExtent, nMax);
  const int ny = getImageDimension(height, maxExtent, nMax);
  const int nz =
      mesh.tetrahedra.empty() ? 1 : getImageDimension(depth, maxExtent, nMax);

  common::ImageStack images({nx, ny, static_cast<std::size_t>(nz)},
                            QImage::Format_RGB32);
  const QRgb nullColor{qRgb(0, 0, 0)};
  images.fill(nullColor);
  images.setVoxelSize(
      {voxelSize(width, nx), voxelSize(height, ny), voxelSize(depth, nz)});

  for (const auto &tet : mesh.tetrahedra) {
    std::array<Point3d, 4> tetVertices;
    for (std::size_t i = 0; i < tetVertices.size(); ++i) {
      if (tet.vertexIndices[i] >= mesh.vertices.size()) {
        SPDLOG_ERROR("Invalid tetrahedron vertex index {}",
                     tet.vertexIndices[i]);
        return {};
      }
      tetVertices[i] = toPoint3d(mesh.vertices[tet.vertexIndices[i]]);
    }
    Point3d bboxMin = tetVertices[0];
    Point3d bboxMax = tetVertices[0];
    for (std::size_t i = 1; i < tetVertices.size(); ++i) {
      bboxMin.x = std::min(bboxMin.x, tetVertices[i].x);
      bboxMin.y = std::min(bboxMin.y, tetVertices[i].y);
      bboxMin.z = std::min(bboxMin.z, tetVertices[i].z);
      bboxMax.x = std::max(bboxMax.x, tetVertices[i].x);
      bboxMax.y = std::max(bboxMax.y, tetVertices[i].y);
      bboxMax.z = std::max(bboxMax.z, tetVertices[i].z);
    }
    const std::array<double, 9> m{tetVertices[1].x - tetVertices[0].x,
                                  tetVertices[2].x - tetVertices[0].x,
                                  tetVertices[3].x - tetVertices[0].x,
                                  tetVertices[1].y - tetVertices[0].y,
                                  tetVertices[2].y - tetVertices[0].y,
                                  tetVertices[3].y - tetVertices[0].y,
                                  tetVertices[1].z - tetVertices[0].z,
                                  tetVertices[2].z - tetVertices[0].z,
                                  tetVertices[3].z - tetVertices[0].z};
    const auto invM = inverse3x3(m);
    if (!invM.has_value()) {
      SPDLOG_ERROR("Degenerate tetrahedron encountered during voxelization");
      return {};
    }
    auto [xMin, xMax] =
        voxelIndexRange(bboxMin.x, bboxMax.x, meshMin.x, width, nx);
    auto [yMin, yMax] =
        voxelIndexRange(bboxMin.y, bboxMax.y, meshMin.y, height, ny);
    auto [zMin, zMax] =
        voxelIndexRange(bboxMin.z, bboxMax.z, meshMin.z, depth, nz);
    auto colorIter = tagToColor.find(tet.physicalTag);
    if (colorIter == tagToColor.cend()) {
      return {};
    }
    if (xMin > xMax || yMin > yMax || zMin > zMax) {
      continue;
    }
    for (int iz = zMin; iz <= zMax; ++iz) {
      auto &img = images[static_cast<std::size_t>(iz)];
      const double z = voxelCenterCoordinate(iz, meshMin.z, depth, nz);
      for (int iy = yMin; iy <= yMax; ++iy) {
        const double y = voxelCenterCoordinate(iy, meshMin.y, height, ny);
        const int py = ny - 1 - iy;
        for (int ix = xMin; ix <= xMax; ++ix) {
          const double x = voxelCenterCoordinate(ix, meshMin.x, width, nx);
          if (pointInsideTetrahedron({x, y, z}, tetVertices, *invM)) {
            img.setPixel(ix, py, colorIter->second);
          }
        }
      }
    }
  }
  if (!mesh.triangles.empty()) {
    auto &img = images[0];
    for (const auto &tri : mesh.triangles) {
      std::array<Point3d, 3> triVertices;
      for (std::size_t i = 0; i < triVertices.size(); ++i) {
        if (tri.vertexIndices[i] >= mesh.vertices.size()) {
          SPDLOG_ERROR("Invalid triangle vertex index {}",
                       tri.vertexIndices[i]);
          return {};
        }
        triVertices[i] = toPoint3d(mesh.vertices[tri.vertexIndices[i]]);
      }
      Point3d bboxMin = triVertices[0];
      Point3d bboxMax = triVertices[0];
      for (std::size_t i = 1; i < triVertices.size(); ++i) {
        bboxMin.x = std::min(bboxMin.x, triVertices[i].x);
        bboxMin.y = std::min(bboxMin.y, triVertices[i].y);
        bboxMax.x = std::max(bboxMax.x, triVertices[i].x);
        bboxMax.y = std::max(bboxMax.y, triVertices[i].y);
      }
      auto [xMin, xMax] =
          voxelIndexRange(bboxMin.x, bboxMax.x, meshMin.x, width, nx);
      auto [yMin, yMax] =
          voxelIndexRange(bboxMin.y, bboxMax.y, meshMin.y, height, ny);
      auto colorIter = tagToColor.find(tri.physicalTag);
      if (colorIter == tagToColor.cend()) {
        return {};
      }
      if (xMin > xMax || yMin > yMax) {
        continue;
      }
      for (int iy = yMin; iy <= yMax; ++iy) {
        const double y = voxelCenterCoordinate(iy, meshMin.y, height, ny);
        const int py = ny - 1 - iy;
        for (int ix = xMin; ix <= xMax; ++ix) {
          const double x = voxelCenterCoordinate(ix, meshMin.x, width, nx);
          if (pointInsideTriangle2d(x, y, triVertices)) {
            img.setPixel(ix, py, colorIter->second);
          }
        }
      }
    }
  }

  if (!includeBackground) {
    fillNullVoxelsWithNearestColor(images, nullColor);
    // Fallback for extreme undersampling where no voxel center lands in a tet.
    fillRemainingNullVoxels(images, nullColor, tagToColor.begin()->second);
  }

  images.convertToIndexed();
  return images;
}

} // namespace sme::mesh
