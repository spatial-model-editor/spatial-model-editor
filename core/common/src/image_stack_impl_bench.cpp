#include "bench.hpp"
#include "sme/image_stack_impl.hpp"

using namespace sme;

static void common_getCombinedColorTable(benchmark::State &state) {
  std::vector<QImage> images;
  sme::common::Volume vol(static_cast<int>(state.range(0)),
                          static_cast<int>(state.range(0)), state.range(0));
  std::vector<QRgb> colors(state.range(1));
  for (int i = 0; i < static_cast<int>(colors.size()); i++) {
    colors[i] = qRgb(i, i + 1, i + 2);
  }
  for (std::size_t z = 0; z < vol.depth(); z++) {
    auto img = QImage(vol.width(), vol.height(), QImage::Format_RGB32);
    for (int y = 0; y < img.height(); ++y) {
      auto *line = reinterpret_cast<QRgb *>(img.scanLine(y));
      for (int x = 0; x < img.width(); ++x) {
        line[x] =
            colors[(z * vol.height() * vol.width() + y * vol.width() + x) %
                   colors.size()];
      }
    }
    images.push_back(img);
  }
  QList<QRgb> colorTable;
  for (auto _ : state) {
    colorTable = common::getCombinedColorTable(images);
  }
  state.SetLabel(fmt::format("{}x{}x{} voxels - {} colors", vol.width(),
                             vol.height(), vol.depth(), colorTable.size()));
}

BENCHMARK(common_getCombinedColorTable)
    ->Args({100, 1})
    ->Args({100, 3})
    ->Args({100, 5})
    ->Args({100, 10})
    ->Args({100, 255})
    ->Args({100, 500})
    ->Args({200, 10})
    ->Args({500, 10})
    ->Args({1000, 10})
    ->Unit(benchmark::kMillisecond);
