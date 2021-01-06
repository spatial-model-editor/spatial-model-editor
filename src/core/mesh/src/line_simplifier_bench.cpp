#include <benchmark/benchmark.h>
#include "boundary.hpp"
#include "line_simplifier.hpp"
#include <QPointF>
#include <QRgb>
#include <QImage>

static void line_simplifier_concave_cell_nucleus_100x100(benchmark::State &state) {
  QImage img(":/geometry/concave-cell-nucleus-100x100.png");
  QRgb col0 = img.pixel(0, 0);
  QRgb col1 = img.pixel(35, 20);
  QRgb col2 = img.pixel(40, 50);
  auto boundaries{
      mesh::constructBoundaries(img, {col0, col1, col2})};
  for (auto _ : state) {
    for(const auto& boundary : boundaries) {
      auto l{mesh::LineSimplifier(boundary.getAllPoints(), boundary.isLoop())};
    }
  }
}

static void line_simplifier_liver_cells_200x100(benchmark::State &state) {
  QImage img(":/geometry/liver-cells-200x100.png");
  QRgb col0 = img.pixel(50, 50);
  QRgb col1 = img.pixel(40, 20);
  QRgb col2 = img.pixel(21, 14);
  auto boundaries{
      mesh::constructBoundaries(img, {col0, col1, col2})};
  for (auto _ : state) {
    for(const auto& boundary : boundaries) {
      auto l{mesh::LineSimplifier(boundary.getAllPoints(), boundary.isLoop())};
    }
  }
}

BENCHMARK(line_simplifier_concave_cell_nucleus_100x100);
BENCHMARK(line_simplifier_liver_cells_200x100);
