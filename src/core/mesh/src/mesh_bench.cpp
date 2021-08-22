#include "bench.hpp"
#include "mesh.hpp"

template <typename T> static void mesh_Mesh(benchmark::State &state) {
  T data;
  for (auto _ : state) {
    auto m{sme::mesh::Mesh(data.img, {}, data.maxTriangleArea, data.pixelWidth,
                           data.origin, data.colours)};
  }
}

template <typename T>
static void mesh_Mesh_getMeshImages(benchmark::State &state) {
  T data;
  QImage img1;
  QImage img2;
  for (auto _ : state) {
    std::tie(img1, img2) = data.mesh.getMeshImages(data.imageSize, 0);
  }
}

template <typename T>
static void mesh_Mesh_getBoundariesImages(benchmark::State &state) {
  T data;
  QImage img1;
  QImage img2;
  for (auto _ : state) {
    std::tie(img1, img2) = data.mesh.getBoundariesImages(data.imageSize, 0);
  }
}

SME_BENCHMARK(mesh_Mesh);
SME_BENCHMARK(mesh_Mesh_getMeshImages);
SME_BENCHMARK(mesh_Mesh_getBoundariesImages);
