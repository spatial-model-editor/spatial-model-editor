#include "bench.hpp"
#include "model_membranes_util.hpp"

template <typename T>
static void model_ImageMembranePixels(benchmark::State &state) {
  T data;
  model::ImageMembranePixels imageMembranePixels;
  for (auto _ : state) {
    imageMembranePixels = model::ImageMembranePixels(data.img);
  }
}

SME_BENCHMARK(model_ImageMembranePixels);
