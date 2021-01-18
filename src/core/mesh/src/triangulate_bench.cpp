#include "boundary.hpp"
#include "interior_point.hpp"
#include "triangulate.hpp"
#include "bench.hpp"

template <typename T> static void
mesh_TriangulateBoundaries(benchmark::State &state) {
  T data;
  auto interiorPoints{sme::mesh::getInteriorPoints(data.img, data.colours)};
  auto boundaries{sme::mesh::constructBoundaries(data.img, data.colours)};
  auto tb{
      sme::mesh::TriangulateBoundaries(boundaries, interiorPoints, data.maxTriangleArea)};
  for (auto _ : state) {
    auto t{sme::mesh::Triangulate(tb)};
  }
}

SME_BENCHMARK(mesh_TriangulateBoundaries);
