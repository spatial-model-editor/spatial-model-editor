#include "bench.hpp"
#include "sme/mesh2d.hpp"

template <typename T> static void mesh_Mesh(benchmark::State &state) {
  T data;
  for (auto _ : state) {
    auto m{sme::mesh::Mesh2d(data.imgs[0], {}, data.maxTriangleArea,
                             data.voxelSize, data.origin, data.colours)};
  }
}

template <typename T>
static void mesh_Mesh_getMeshImages(benchmark::State &state) {
  T data;
  sme::common::ImageStack img1;
  sme::common::ImageStack img2;
  for (auto _ : state) {
    std::tie(img1, img2) = data.mesh.getMeshImages(data.imageSize, 0);
  }
}

template <typename T>
static void mesh_Mesh_getBoundariesImages(benchmark::State &state) {
  T data;
  sme::common::ImageStack img1;
  sme::common::ImageStack img2;
  for (auto _ : state) {
    std::tie(img1, img2) = data.mesh.getBoundariesImages(data.imageSize, 0);
  }
}

SME_BENCHMARK(mesh_Mesh);
SME_BENCHMARK(mesh_Mesh_getMeshImages);
SME_BENCHMARK(mesh_Mesh_getBoundariesImages);
