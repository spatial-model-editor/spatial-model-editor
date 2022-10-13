#include "bench.hpp"
#include "interior_point.hpp"

template <typename T>
static void mesh_getInteriorPoints(benchmark::State &state) {
  T data;
  std::vector<std::vector<QPointF>> interiorPoints;
  for (auto _ : state) {
    interiorPoints = sme::mesh::getInteriorPoints(data.imgs[0], data.colours);
  }
}

SME_BENCHMARK(mesh_getInteriorPoints);
