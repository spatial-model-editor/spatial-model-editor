#include "catch_wrapper.hpp"
#include "imagerenderutils.hpp"
#include <QColor>
#include <algorithm>
#include <cmath>
#include <vector>

using namespace sme::gui;

TEST_CASE("ImageRenderUtils preserves physical aspect ratio",
          "[gui/widgets/imagerenderutils][gui/widgets][gui]") {
  QImage img(4, 4, QImage::Format_RGB32);
  img.fill(Qt::white);

  ImageRenderOptions opts;
  opts.aspectRatioMode = Qt::KeepAspectRatio;
  opts.physicalSize = {4.0, 2.0, 1.0};
  auto rendered = renderImageWithOverlays(img, {100, 100}, opts);
  REQUIRE(rendered.offset == QPoint(0, 0));
  REQUIRE(rendered.pixmapImageSize == QSize(100, 50));

  opts.aspectRatioMode = Qt::IgnoreAspectRatio;
  rendered = renderImageWithOverlays(img, {100, 100}, opts);
  REQUIRE(rendered.pixmapImageSize == QSize(100, 100));
}

TEST_CASE("ImageRenderUtils scale uses origin in labels",
          "[gui/widgets/imagerenderutils][gui/widgets][gui]") {
  QImage img(16, 16, QImage::Format_RGB32);
  img.fill(Qt::black);

  ImageRenderOptions opts;
  opts.drawScale = true;
  opts.lengthUnits = "m";
  opts.physicalSize = {16.0, 16.0, 1.0};

  opts.physicalOrigin = {0.0, 0.0, 0.0};
  auto zeroOrigin = renderImageWithOverlays(img, {220, 220}, opts);

  opts.physicalOrigin = {7.5, 3.25, 0.0};
  auto offsetOrigin = renderImageWithOverlays(img, {220, 220}, opts);

  REQUIRE(zeroOrigin.offset.x() > 0);
  REQUIRE(zeroOrigin.offset.y() > 0);
  REQUIRE(zeroOrigin.pixmap.toImage() != offsetOrigin.pixmap.toImage());
}

TEST_CASE("ImageRenderUtils draws vertical indicator",
          "[gui/widgets/imagerenderutils][gui/widgets][gui]") {
  QImage img(4, 3, QImage::Format_RGB32);
  img.fill(Qt::white);

  ImageRenderOptions opts;
  opts.aspectRatioMode = Qt::IgnoreAspectRatio;
  auto noIndicator = renderImageWithOverlays(img, {120, 40}, opts);

  opts.verticalIndicatorSourceX = 1;
  auto withIndicator = renderImageWithOverlays(img, {120, 40}, opts);

  auto getBlackColumns = [](const QImage &rendered) {
    std::vector<int> cols;
    int y{rendered.height() / 2};
    for (int x = 0; x < rendered.width(); ++x) {
      if (rendered.pixelColor(x, y) == QColor(Qt::black)) {
        cols.push_back(x);
      }
    }
    return cols;
  };

  REQUIRE(getBlackColumns(noIndicator.pixmap.toImage()).empty());
  auto blackColumns = getBlackColumns(withIndicator.pixmap.toImage());
  REQUIRE(blackColumns.size() == 1);
  int expectedX{std::clamp(
      static_cast<int>(std::lround(
          1.5 * static_cast<double>(withIndicator.pixmapImageSize.width()) /
          static_cast<double>(img.width()))) -
          1,
      0, withIndicator.pixmapImageSize.width() - 1)};
  REQUIRE(blackColumns[0] == expectedX);
}

TEST_CASE("ImageRenderUtils grid aligns to source pixels",
          "[gui/widgets/imagerenderutils][gui/widgets][gui]") {
  QImage img(5, 5, QImage::Format_RGB32);
  img.fill(Qt::black);

  ImageRenderOptions opts;
  opts.drawGrid = true;
  opts.physicalSize = {5.0, 5.0, 1.0};
  auto rendered =
      renderImageWithOverlays(img, {200, 200}, opts).pixmap.toImage();

  int midY{rendered.height() / 2};
  std::vector<int> gridColumns;
  auto isGridColor = [](const QColor &c) {
    return c.red() == 127 && c.green() == 127 && c.blue() == 127;
  };
  bool inColumn{false};
  for (int x = 0; x < rendered.width(); ++x) {
    bool isGrid{isGridColor(rendered.pixelColor(x, midY))};
    if (isGrid && !inColumn) {
      gridColumns.push_back(x);
      inColumn = true;
    } else if (!isGrid && inColumn) {
      inColumn = false;
    }
  }

  std::vector<int> gridSourceColumns;
  for (int x : gridColumns) {
    int srcX{
        static_cast<int>(std::lround(static_cast<double>(x * img.width()) /
                                     static_cast<double>(rendered.width())))};
    int expectedX{static_cast<int>(
        std::lround(static_cast<double>(srcX * rendered.width()) /
                    static_cast<double>(img.width())))};
    REQUIRE(x == expectedX);
    if (gridSourceColumns.empty() || srcX != gridSourceColumns.back()) {
      gridSourceColumns.push_back(srcX);
    }
  }
  REQUIRE(gridSourceColumns.size() == 5);
}

TEST_CASE("ImageRenderUtils scale tick marks align with grid lines",
          "[gui/widgets/imagerenderutils][gui/widgets][gui]") {
  QImage img(100, 100, QImage::Format_RGB32);
  img.fill(Qt::black);

  ImageRenderOptions opts;
  opts.drawGrid = true;
  opts.drawScale = true;
  opts.physicalSize = {100.0, 100.0, 1.0};
  auto rendered = renderImageWithOverlays(img, {220, 220}, opts);
  auto pixmap = rendered.pixmap.toImage();

  auto isGridColor = [](const QColor &c) {
    return c.red() == 127 && c.green() == 127 && c.blue() == 127;
  };
  auto getLinePositions = [&](int fixedCoord, bool horizontalScan) {
    std::vector<int> positions;
    bool inLine{false};
    int limit{horizontalScan ? pixmap.width() : pixmap.height()};
    for (int i = 0; i < limit; ++i) {
      QColor color{horizontalScan ? pixmap.pixelColor(i, fixedCoord)
                                  : pixmap.pixelColor(fixedCoord, i)};
      bool isLine{isGridColor(color)};
      if (isLine && !inLine) {
        positions.push_back(i);
        inLine = true;
      } else if (!isLine && inLine) {
        inLine = false;
      }
    }
    return positions;
  };

  auto gridColumns{
      getLinePositions(rendered.pixmapImageSize.height() / 2, true)};
  auto tickColumns{getLinePositions(
      rendered.pixmapImageSize.height() - 1 + opts.tickLength / 2, true)};
  REQUIRE(!gridColumns.empty());
  // The x-axis may omit the final edge tick if its label would overlap.
  REQUIRE(tickColumns.size() >= gridColumns.size());
  for (int x : gridColumns) {
    REQUIRE(std::find(tickColumns.cbegin(), tickColumns.cend(), x) !=
            tickColumns.cend());
  }

  auto gridRows{getLinePositions(
      rendered.offset.x() + rendered.pixmapImageSize.width() / 2, false)};
  auto tickRows{
      getLinePositions(rendered.offset.x() - opts.tickLength / 2, false)};
  REQUIRE(!gridRows.empty());
  REQUIRE(tickRows.size() == gridRows.size() + 1);
  for (int y : gridRows) {
    REQUIRE(std::find(tickRows.cbegin(), tickRows.cend(), y) !=
            tickRows.cend());
  }
}
