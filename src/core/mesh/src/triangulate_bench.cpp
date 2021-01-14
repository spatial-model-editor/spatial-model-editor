#include "boundary.hpp"
#include "interior_point.hpp"
#include "triangulate.hpp"
#include <QImage>
#include <QPointF>
#include <QRgb>
#include <benchmark/benchmark.h>

static void triangulate_concave_cell_nucleus_100x100(benchmark::State &state) {
  QImage img(":/geometry/concave-cell-nucleus-100x100.png");
  QRgb col0 = img.pixel(0, 0);
  QRgb col1 = img.pixel(35, 20);
  QRgb col2 = img.pixel(40, 50);
  auto interiorPoints{mesh::getInteriorPoints(img, {col0, col1, col2})};
  auto boundaries{mesh::constructBoundaries(img, {col0, col1, col2})};
  auto tb{
      mesh::TriangulateBoundaries(boundaries, interiorPoints, {10, 10, 10})};
  for (auto _ : state) {
    auto t{mesh::Triangulate(tb)};
  }
}

static void triangulate_boundary_liver_cells_200x100(benchmark::State &state) {
  QImage img(":/geometry/liver-cells-200x100.png");
  QRgb col0 = img.pixel(50, 50);
  QRgb col1 = img.pixel(40, 20);
  QRgb col2 = img.pixel(21, 14);
  auto interiorPoints{mesh::getInteriorPoints(img, {col0, col1, col2})};
  auto boundaries{mesh::constructBoundaries(img, {col0, col1, col2})};
  for (auto _ : state) {
    auto tb{
        mesh::TriangulateBoundaries(boundaries, interiorPoints, {10, 10, 10})};
    auto t{mesh::Triangulate(tb)};
  }
}

BENCHMARK(triangulate_concave_cell_nucleus_100x100)->Unit(benchmark::kMillisecond);
BENCHMARK(triangulate_boundary_liver_cells_200x100)->Unit(benchmark::kMillisecond);
