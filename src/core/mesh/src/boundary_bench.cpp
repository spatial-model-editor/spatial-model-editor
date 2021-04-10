#include "bench.hpp"
#include "boundary.hpp"

template <typename T>
static void mesh_constructBoundaries(benchmark::State &state) {
  T data;
  std::vector<sme::mesh::Boundary> boundaries;
  QImage pixelCorners;
  for (auto _ : state) {
    boundaries =
        sme::mesh::constructBoundaries(data.img, data.colours, pixelCorners);
  }
}

template <typename T>
static void mesh_Boundary_setMaxPoints(benchmark::State &state) {
  T data;
  QImage pixelCorners;
  auto boundaries{
      sme::mesh::constructBoundaries(data.img, data.colours, pixelCorners)};
  auto nPoints{boundaries[0].getMaxPoints()};
  for (auto _ : state) {
    boundaries[0].setMaxPoints(nPoints + 1);
  }
}

SME_BENCHMARK(mesh_constructBoundaries);
SME_BENCHMARK(mesh_Boundary_setMaxPoints);
