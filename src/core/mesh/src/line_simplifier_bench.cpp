#include "bench.hpp"
#include "boundaries.hpp"
#include "line_simplifier.hpp"

template <typename T> static void mesh_LineSimplifier(benchmark::State &state) {
  T data;
  sme::mesh::Boundaries boundaries(data.img, data.colours);
  for (auto _ : state) {
    for (const auto &boundary : boundaries.getBoundaries()) {
      auto l{sme::mesh::LineSimplifier(boundary.getAllPoints(),
                                       boundary.isLoop())};
    }
  }
}

SME_BENCHMARK(mesh_LineSimplifier);
