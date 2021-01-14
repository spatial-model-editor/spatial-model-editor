#include "boundary.hpp"
#include "line_simplifier.hpp"
#include "bench.hpp"

template <typename T> static void mesh_LineSimplifier(benchmark::State &state) {
  T data;
  auto boundaries{
      mesh::constructBoundaries(data.img, data.colours)};
  for (auto _ : state) {
    for(const auto& boundary : boundaries) {
      auto l{mesh::LineSimplifier(boundary.getAllPoints(), boundary.isLoop())};
    }
  }
}

SME_BENCHMARK(mesh_LineSimplifier);
