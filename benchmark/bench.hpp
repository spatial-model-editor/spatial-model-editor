#pragma once

#include "sme/image_stack.hpp"
#include "sme/mesh2d.hpp"
#include "sme/model.hpp"
#include "sme/model_membranes_util.hpp"
#include <QFile>
#include <QImage>
#include <QPointF>
#include <QRgb>
#include <benchmark/benchmark.h>
#include <utility>

// Each dataset for constructing benchmarks is stored as a custom type
struct ABtoC {
  sme::common::ImageStack imgs{{QImage{":/geometry/circle-100x100.png"}}};
  std::vector<QRgb> colours{imgs[0].pixel(1, 1), imgs[0].pixel(50, 50)};
  std::vector<std::size_t> maxTriangleArea{10, 10};
  sme::common::VoxelF origin{0.0, 0.0, 0.0};
  sme::common::VolumeF voxelSize{1.0, 1.0, 1.0};
  sme::mesh::Mesh2d mesh{imgs[0],   {},     maxTriangleArea,
                         voxelSize, origin, colours};
  QSize imageSize{100, 100};
  sme::model::ImageMembranePixels imageMembranePixels{imgs};
  std::pair<std::size_t, std::size_t> membraneIndexPair{0, 1};
  sme::model::Model model;
  std::string xml;
  ABtoC() {
    QFile f(":/models/ABtoC.xml");
    f.open(QIODevice::ReadOnly);
    xml = f.readAll().toStdString();
    model.importSBMLString(xml);
  }
};

struct VerySimpleModel {
  sme::common::ImageStack imgs{
      {QImage{":/geometry/concave-cell-nucleus-100x100.png"}}};
  std::vector<QRgb> colours{imgs[0].pixel(0, 0), imgs[0].pixel(35, 20),
                            imgs[0].pixel(40, 50)};
  std::vector<std::size_t> maxTriangleArea{10, 10, 10};
  sme::common::VoxelF origin{0.0, 0.0, 0.0};
  sme::common::VolumeF voxelSize{1.0, 1.0, 1.0};
  sme::mesh::Mesh2d mesh{imgs[0],   {},     maxTriangleArea,
                         voxelSize, origin, colours};
  QSize imageSize{300, 300};
  sme::model::ImageMembranePixels imageMembranePixels{imgs};
  std::pair<std::size_t, std::size_t> membraneIndexPair{0, 1};
  sme::model::Model model;
  std::string xml;
  VerySimpleModel() {
    QFile f(":/models/very-simple-model.xml");
    f.open(QIODevice::ReadOnly);
    xml = f.readAll().toStdString();
    model.importSBMLString(xml);
  }
};

struct LiverCells {
  sme::common::ImageStack imgs{{QImage{":/geometry/liver-cells-200x100.png"}}};
  std::vector<QRgb> colours{imgs[0].pixel(50, 50), imgs[0].pixel(40, 20),
                            imgs[0].pixel(21, 14)};
  std::vector<std::size_t> maxTriangleArea{2, 2, 2};
  sme::common::VoxelF origin{0.0, 0.0, 0.0};
  sme::common::VolumeF voxelSize{1.0, 1.0, 1.0};
  sme::mesh::Mesh2d mesh{imgs[0],   {},     maxTriangleArea,
                         voxelSize, origin, colours};
  QSize imageSize{1000, 1000};
  sme::model::ImageMembranePixels imageMembranePixels{imgs};
  std::pair<std::size_t, std::size_t> membraneIndexPair{0, 1};
  sme::model::Model model;
  std::string xml;
  LiverCells() {
    QFile f(":/models/liver-cells.xml");
    f.open(QIODevice::ReadOnly);
    xml = f.readAll().toStdString();
    model.importSBMLString(xml);
  }
};

// Use millisecond as the default time unit
#define SME_BENCHMARK_TEMPLATE(X, data)                                        \
  BENCHMARK_TEMPLATE(X, data)->Unit(benchmark::kMillisecond)

// Given a benchmark function, for each dataset defined above, this macro
// creates a benchmark with the dataset supplied as a template parameter
#define SME_BENCHMARK(func)                                                    \
  SME_BENCHMARK_TEMPLATE(func, ABtoC);                                         \
  SME_BENCHMARK_TEMPLATE(func, VerySimpleModel);                               \
  SME_BENCHMARK_TEMPLATE(func, LiverCells)
