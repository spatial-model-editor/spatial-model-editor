#include "bench.hpp"
#include "boundaries.hpp"

template <typename T> static void mesh_Boundaries(benchmark::State &state) {
  T data;
  sme::mesh::Boundaries boundaries;
  for (auto _ : state) {
    boundaries = sme::mesh::Boundaries(data.img, data.colours);
  }
}

template <typename T>
static void mesh_Boundary_setMaxPoints(benchmark::State &state) {
  T data;
  sme::mesh::Boundaries boundaries(data.img, data.colours);
  auto nPoints{boundaries.getMaxPoints(0)};
  for (auto _ : state) {
    boundaries.setMaxPoints(0, nPoints + 1);
  }
}

SME_BENCHMARK(mesh_Boundaries);
SME_BENCHMARK(mesh_Boundary_setMaxPoints);
