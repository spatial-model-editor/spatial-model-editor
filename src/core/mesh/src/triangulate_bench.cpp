#include "bench.hpp"
#include "boundary.hpp"
#include "interior_point.hpp"
#include "triangulate.hpp"

template <typename T>
static void mesh_TriangulateBoundaries(benchmark::State &state) {
  T data;
  auto interiorPoints{sme::mesh::getInteriorPoints(data.img, data.colours)};
  auto boundaries{sme::mesh::constructBoundaries(data.img, data.colours)};
  for (auto _ : state) {
    auto t{sme::mesh::Triangulate(boundaries, interiorPoints,
                                  data.maxTriangleArea)};
  }
}

SME_BENCHMARK(mesh_TriangulateBoundaries);
