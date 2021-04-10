#include "bench.hpp"
#include "model_membranes_util.hpp"

using namespace sme;

template <typename T>
static void model_ImageMembranePixels(benchmark::State &state) {
  T data;
  std::unique_ptr<model::ImageMembranePixels> imageMembranePixels;
  for (auto _ : state) {
    imageMembranePixels = std::make_unique<model::ImageMembranePixels>(
        data.img, data.mesh.getPixelCornersImage());
  }
}

SME_BENCHMARK(model_ImageMembranePixels);
