#include "bench.hpp"
#include "boundaries.hpp"
#include "interior_point.hpp"
#include "triangulate.hpp"

template <typename T>
static void mesh_TriangulateBoundaries(benchmark::State &state) {
  T data;
  auto interiorPoints{sme::mesh::getInteriorPoints(data.img, data.colours)};
  sme::mesh::Boundaries boundaries(data.img, data.colours);
  for (auto _ : state) {
    auto t{sme::mesh::Triangulate(boundaries.getBoundaries(), interiorPoints,
                                  data.maxTriangleArea)};
  }
}

SME_BENCHMARK(mesh_TriangulateBoundaries);
