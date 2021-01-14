#pragma once

#include "mesh.hpp"
#include <QImage>
#include <QPointF>
#include <QRgb>
#include <benchmark/benchmark.h>

// Each dataset for constructing benchmarks is stored as a custom type
struct ConcaveCellNucleus_100x100 {
  QImage img{":/geometry/concave-cell-nucleus-100x100.png"};
  std::vector<QRgb> colours{img.pixel(0, 0), img.pixel(35, 20),
                            img.pixel(40, 50)};
  std::vector<std::size_t> maxTriangleArea{10, 10, 10};
  QPointF origin{0.0, 0.0};
  double pixelWidth{1.0};
  mesh::Mesh mesh{img, {}, maxTriangleArea, pixelWidth, origin, colours};
  QSize imageSize{100, 100};
};

struct LiverCells_200x100 {
  QImage img{":/geometry/liver-cells-200x100.png"};
  std::vector<QRgb> colours{img.pixel(50, 50), img.pixel(40, 20),
                            img.pixel(21, 14)};
  std::vector<std::size_t> maxTriangleArea{2, 2, 2};
  QPointF origin{0.0, 0.0};
  double pixelWidth{1.0};
  mesh::Mesh mesh{img, {}, maxTriangleArea, pixelWidth, origin, colours};
  QSize imageSize{1000, 1000};
};

// Use millisecond as the default time unit
#define SME_BENCHMARK_TEMPLATE(X, data)                                        \
  BENCHMARK_TEMPLATE(X, data)->Unit(benchmark::kMillisecond);

// Given a benchmark function, for each dataset defined above, this macro
// creates a benchmark with the dataset supplied as a template parameter
#define SME_BENCHMARK(func)                                                        \
  SME_BENCHMARK_TEMPLATE(func, ConcaveCellNucleus_100x100);                    \
  SME_BENCHMARK_TEMPLATE(func, LiverCells_200x100);
