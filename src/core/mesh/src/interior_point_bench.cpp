#include "interior_point.hpp"
#include <QImage>
#include <QPointF>
#include <QRgb>
#include <benchmark/benchmark.h>

static void
interior_points_concave_cell_nucleus_100x100(benchmark::State &state) {
  QImage img(":/geometry/concave-cell-nucleus-100x100.png");
  QRgb col0 = img.pixel(0, 0);
  QRgb col1 = img.pixel(35, 20);
  QRgb col2 = img.pixel(40, 50);
  std::vector<std::vector<QPointF>> interiorPoints;
  for (auto _ : state) {
    interiorPoints = mesh::getInteriorPoints(img, {col0, col1, col2});
  }
}

static void
interior_points_boundary_liver_cells_200x100(benchmark::State &state) {
  QImage img(":/geometry/liver-cells-200x100.png");
  QRgb col0 = img.pixel(50, 50);
  QRgb col1 = img.pixel(40, 20);
  QRgb col2 = img.pixel(21, 14);
  std::vector<std::vector<QPointF>> interiorPoints;
  for (auto _ : state) {
    interiorPoints = mesh::getInteriorPoints(img, {col0, col1, col2});
  }
}

BENCHMARK(interior_points_concave_cell_nucleus_100x100)->Unit(benchmark::kMillisecond);
BENCHMARK(interior_points_boundary_liver_cells_200x100)->Unit(benchmark::kMillisecond);
