#include "bench.hpp"
#include "boundary.hpp"
#include "line_simplifier.hpp"

template <typename T> static void mesh_LineSimplifier(benchmark::State &state) {
  T data;
  QImage pixelCorners;
  auto boundaries{
      sme::mesh::constructBoundaries(data.img, data.colours, pixelCorners)};
  for (auto _ : state) {
    for (const auto &boundary : boundaries) {
      auto l{sme::mesh::LineSimplifier(boundary.getAllPoints(),
                                       boundary.isLoop())};
    }
  }
}

SME_BENCHMARK(mesh_LineSimplifier);
