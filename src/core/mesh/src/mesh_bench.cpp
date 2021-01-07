#include "mesh.hpp"
#include <QImage>
#include <QPointF>
#include <QRgb>
#include <benchmark/benchmark.h>

static void mesh_concave_cell_nucleus_100x100(benchmark::State &state) {
  QImage img(":/geometry/concave-cell-nucleus-100x100.png");
  QRgb col0 = img.pixel(0, 0);
  QRgb col1 = img.pixel(35, 20);
  QRgb col2 = img.pixel(40, 50);
  for (auto _ : state) {
    auto m{
        mesh::Mesh(img, {}, {10, 10, 10}, 1.0, {0.0, 0.0}, {col0, col1, col2})};
  }
}

static void mesh_liver_cells_200x100(benchmark::State &state) {
  QImage img(":/geometry/liver-cells-200x100.png");
  QRgb col0 = img.pixel(50, 50);
  QRgb col1 = img.pixel(40, 20);
  QRgb col2 = img.pixel(21, 14);
  for (auto _ : state) {
    auto m{
        mesh::Mesh(img, {}, {10, 10, 10}, 1.0, {0.0, 0.0}, {col0, col1, col2})};
  }
}

BENCHMARK(mesh_concave_cell_nucleus_100x100);
BENCHMARK(mesh_liver_cells_200x100);
