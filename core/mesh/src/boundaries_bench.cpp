#include "bench.hpp"
#include "boundaries.hpp"

template <typename T> static void mesh_Boundaries(benchmark::State &state) {
  T data;
  sme::mesh::Boundaries boundaries;
  for (auto _ : state) {
    boundaries = sme::mesh::Boundaries(data.img, data.colours, 0);
  }
}

template <typename T>
static void mesh_Boundary_setMaxPoints0(benchmark::State &state) {
  T data;
  sme::mesh::Boundaries boundaries(data.img, data.colours, 0);
  auto nPoints{boundaries.getMaxPoints(0)};
  for (auto _ : state) {
    boundaries.setMaxPoints(0, nPoints + 1);
  }
}

template <typename T>
static void mesh_Boundary_setMaxPoints1(benchmark::State &state) {
  T data;
  sme::mesh::Boundaries boundaries(data.img, data.colours, 1);
  auto nPoints{boundaries.getMaxPoints(0)};
  for (auto _ : state) {
    boundaries.setMaxPoints(0, nPoints + 1);
  }
}

SME_BENCHMARK(mesh_Boundaries);
SME_BENCHMARK(mesh_Boundary_setMaxPoints0);
SME_BENCHMARK(mesh_Boundary_setMaxPoints1);
