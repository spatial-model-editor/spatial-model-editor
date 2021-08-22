#include "bench.hpp"
#include "boundary.hpp"

template <typename T>
static void mesh_constructBoundaries(benchmark::State &state) {
  T data;
  std::vector<sme::mesh::Boundary> boundaries;
  data.img.save("QQQ.png");
  for (auto _ : state) {
    boundaries = sme::mesh::constructBoundaries(data.img, data.colours);
  }
}

template <typename T>
static void mesh_Boundary_setMaxPoints(benchmark::State &state) {
  T data;
  auto boundaries{sme::mesh::constructBoundaries(data.img, data.colours)};
  auto nPoints{boundaries[0].getMaxPoints()};
  for (auto _ : state) {
    boundaries[0].setMaxPoints(nPoints + 1);
  }
}

SME_BENCHMARK(mesh_constructBoundaries);
SME_BENCHMARK(mesh_Boundary_setMaxPoints);
