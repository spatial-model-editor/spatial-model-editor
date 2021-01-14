#include "geometry.hpp"
#include <benchmark/benchmark.h>

static void compartment_concave_cell_nucleus_100x100(benchmark::State &state) {
  QImage img(":/geometry/concave-cell-nucleus-100x100.png");
  QRgb col0{img.pixel(0, 0)};
  geometry::Compartment compartment;
  for (auto _ : state) {
      compartment = geometry::Compartment("id", img, col0);
    }
  }

static void compartment_liver_cells_200x100(benchmark::State &state) {
  QImage img(":/geometry/liver-cells-200x100.png");
  QRgb col0 {img.pixel(50, 50)};
  geometry::Compartment compartment;
  for (auto _ : state) {
      compartment = geometry::Compartment("id", img, col0);
  }
}

static void concentration_image_array_concave_cell_nucleus_100x100(benchmark::State &state) {
  QImage img(":/geometry/concave-cell-nucleus-100x100.png");
  QRgb col0{img.pixel(0, 0)};
  geometry::Compartment compartment("id", img, col0);
  geometry::Field field(&compartment, "id", 1.0, col0);
  field.setUniformConcentration(1.2);
  std::vector<double> concentration;
  for (auto _ : state) {
    concentration = field.getConcentrationImageArray();
  }
}

static void concentration_image_array_liver_cells_200x100(benchmark::State &state) {
  QImage img(":/geometry/liver-cells-200x100.png");
  QRgb col0 {img.pixel(50, 50)};
  geometry::Compartment compartment("id", img, col0);
  geometry::Field field(&compartment, "id", 1.0, col0);
  field.setUniformConcentration(1.2);
  std::vector<double> concentration;
  for (auto _ : state) {
    concentration = field.getConcentrationImageArray();
  }
}

BENCHMARK(compartment_concave_cell_nucleus_100x100)->Unit(benchmark::kMillisecond);
BENCHMARK(compartment_liver_cells_200x100)->Unit(benchmark::kMillisecond);
BENCHMARK(concentration_image_array_concave_cell_nucleus_100x100)->Unit(benchmark::kMillisecond);
BENCHMARK(concentration_image_array_liver_cells_200x100)->Unit(benchmark::kMillisecond);
