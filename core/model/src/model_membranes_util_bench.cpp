#include "bench.hpp"
#include "sme/model_membranes_util.hpp"

using namespace sme;

template <typename T>
static void model_ImageMembranePixels(benchmark::State &state) {
  T data;
  model::ImageMembranePixels imageMembranePixels;
  for (auto _ : state) {
    imageMembranePixels = model::ImageMembranePixels(data.imgs);
  }
}

SME_BENCHMARK(model_ImageMembranePixels);
