#include "boundary.hpp"
#include "bench.hpp"

template <typename T> static void mesh_constructBoundaries(benchmark::State &state) {
  T data;
  std::vector<mesh::Boundary> boundaries;
  for (auto _ : state) {
    boundaries = mesh::constructBoundaries(data.img, data.colours);
  }
}

SME_BENCHMARK(mesh_constructBoundaries);
