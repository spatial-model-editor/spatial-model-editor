#include "imagerenderutils.hpp"
#include <QPainter>
#include <algorithm>
#include <cmath>
#include <vector>

namespace sme::gui {

namespace {

struct GridSpacing {
  double physicalWidth{0.0};
  int pixelStep{1};
};

static GridSpacing getGridWidth(const common::VolumeF &physicalSize,
                                const QSize &pixmapSize,
                                const QSize &imagePixelSize) {
  constexpr double minWidthPixels{20};
  // start with grid of width 1 physical unit
  double gridPixelWidth{static_cast<double>(pixmapSize.width())};
  double gridPhysicalWidth{physicalSize.width()};
  // rescale width in pixels to: ~ [1, 2] * minWidthPixels
  while (gridPixelWidth < minWidthPixels) {
    gridPhysicalWidth *= 10.0;
    gridPixelWidth *= 10.0;
  }
  while (gridPixelWidth > 10 * minWidthPixels) {
    gridPhysicalWidth /= 10.0;
    gridPixelWidth /= 10.0;
  }
  if (gridPixelWidth > 8 * minWidthPixels) {
    gridPhysicalWidth /= 5.0;
    gridPixelWidth /= 5.0;
  } else if (gridPixelWidth > 4 * minWidthPixels) {
    gridPhysicalWidth /= 2.0;
    gridPixelWidth /= 2.0;
  }
  int pixelStep{1};
  if (imagePixelSize.width() > 0 && pixmapSize.width() > 0) {
    double pixelDisplayWidth{static_cast<double>(pixmapSize.width()) /
                             static_cast<double>(imagePixelSize.width())};
    if (pixelDisplayWidth > 0.0) {
      double pixelPhysicalWidth{physicalSize.width() /
                                static_cast<double>(imagePixelSize.width())};
      double multiple{std::round(gridPixelWidth / pixelDisplayWidth)};
      if (multiple < 1.0) {
        multiple = 1.0;
      }
      pixelStep = static_cast<int>(multiple);
      gridPhysicalWidth = multiple * pixelPhysicalWidth;
    }
  }
  return {gridPhysicalWidth, pixelStep};
}

static QString getGridPointLabel(double value, bool includeUnits,
                                 const QString &lengthUnits) {
  auto label{QString::number(value, 'g', 3)};
  if (includeUnits) {
    label.append(" ").append(lengthUnits);
  }
  return label;
}

static std::vector<int> getGridEdgePixels(int nPixels, int pixelStep) {
  std::vector<int> edgePixels;
  if (nPixels <= 0 || pixelStep <= 0) {
    return edgePixels;
  }
  for (int i = 0; i < nPixels; i += pixelStep) {
    edgePixels.push_back(i);
  }
  return edgePixels;
}

// Returns pixel positions for scale tick marks, from 0 to nPixels (inclusive).
// nPixels is one past the last valid pixel index: it represents the far edge
// of the image and is always included so the full physical extent is labelled.
static std::vector<int> getScaleEdgePixels(int nPixels, int pixelStep) {
  if (nPixels <= 0 || pixelStep <= 0) {
    return {};
  }
  auto edgePixels{getGridEdgePixels(nPixels, pixelStep)};
  if (edgePixels.back() != nPixels) {
    edgePixels.push_back(nPixels);
  }
  return edgePixels;
}

static int sourceXToPixmapX(int srcX, int pixmapWidth, int imageWidth) {
  return std::clamp(
      static_cast<int>(std::lround(static_cast<double>(srcX) *
                                   static_cast<double>(pixmapWidth) /
                                   static_cast<double>(imageWidth))),
      0, pixmapWidth - 1);
}

static int sourceYToPixmapY(int srcY, int pixmapHeight, int imageHeight,
                            bool flipYAxis) {
  if (flipYAxis) {
    return std::clamp(
        static_cast<int>(std::lround(static_cast<double>(srcY) *
                                     static_cast<double>(pixmapHeight) /
                                     static_cast<double>(imageHeight))),
        0, pixmapHeight - 1);
  }
  return std::clamp(
      static_cast<int>(std::lround(static_cast<double>(imageHeight - srcY) *
                                   static_cast<double>(pixmapHeight) /
                                   static_cast<double>(imageHeight))),
      0, pixmapHeight - 1);
}

static QSize getPixmapSize(const QSize &displaySize, double physicalAspectRatio,
                           Qt::AspectRatioMode aspectRatioMode) {
  int width{std::max(1, displaySize.width())};
  int height{std::max(1, displaySize.height())};
  if (aspectRatioMode == Qt::IgnoreAspectRatio ||
      !std::isfinite(physicalAspectRatio) || physicalAspectRatio <= 0.0) {
    return {width, height};
  }
  double displayAspectRatio{static_cast<double>(displaySize.width()) /
                            static_cast<double>(displaySize.height())};
  if (displayAspectRatio > physicalAspectRatio) {
    return {std::max(1, static_cast<int>(displaySize.height() *
                                         physicalAspectRatio)),
            height};
  }
  return {width, std::max(1, static_cast<int>(displaySize.width() /
                                              physicalAspectRatio))};
}

} // namespace

RenderedImage renderImageWithOverlays(const QImage &srcImage,
                                      const QSize &displaySize,
                                      const ImageRenderOptions &options) {
  if (srcImage.isNull()) {
    return {};
  }

  RenderedImage rendered;
  rendered.pixmap = QPixmap(displaySize);
  rendered.pixmap.fill(QColor(0, 0, 0, 0));
  QPainter p(&rendered.pixmap);
  bool haveSpaceForScale{false};
  if (options.drawScale) {
    int w{p.fontMetrics().horizontalAdvance("8e+88")};
    int h{p.fontMetrics().height()};
    // only draw scale if picture is bigger than labels
    if (displaySize.width() > 2 * w && displaySize.height() > 2 * h) {
      rendered.offset = {w + options.tickLength, h + options.tickLength};
      haveSpaceForScale = true;
    }
  }
  QSize availableSize{displaySize};
  availableSize.rwidth() -= rendered.offset.x();
  availableSize.rheight() -= rendered.offset.y();
  rendered.pixmapImageSize = getPixmapSize(availableSize,
                                           options.physicalSize.width() /
                                               options.physicalSize.height(),
                                           options.aspectRatioMode);
  double sx{static_cast<double>(rendered.pixmapImageSize.width()) /
            static_cast<double>(srcImage.width())};
  double sy{static_cast<double>(rendered.pixmapImageSize.height()) /
            static_cast<double>(srcImage.height())};
  p.setRenderHint(QPainter::SmoothPixmapTransform,
                  options.transformationMode == Qt::SmoothTransformation);
  p.save();
  p.translate(rendered.offset.x(), 0);
  p.scale(sx, sy);
  p.drawImage(QPoint(0, 0), srcImage);
  p.restore();
  if (options.drawGrid || (options.drawScale && haveSpaceForScale)) {
    QPen gridPen(QColor(127, 127, 127));
    gridPen.setCosmetic(true);
    p.setPen(gridPen);
    auto gridSpacing = getGridWidth(options.physicalSize,
                                    rendered.pixmapImageSize, srcImage.size());
    int prevTextEnd{-1000};
    auto xGridEdgePixels{
        getGridEdgePixels(srcImage.width(), gridSpacing.pixelStep)};
    if (options.drawGrid) {
      for (int srcX : xGridEdgePixels) {
        int x{sourceXToPixmapX(srcX, rendered.pixmapImageSize.width(),
                               srcImage.width())};
        p.drawLine(x + rendered.offset.x(), 0, x + rendered.offset.x(),
                   rendered.pixmapImageSize.height() - 1);
      }
    }
    auto xScaleEdgePixels{
        getScaleEdgePixels(srcImage.width(), gridSpacing.pixelStep)};
    double xPixelWidth{options.physicalSize.width() /
                       static_cast<double>(srcImage.width())};
    for (int srcX : xScaleEdgePixels) {
      int x{sourceXToPixmapX(srcX, rendered.pixmapImageSize.width(),
                             srcImage.width())};
      if (options.drawScale && haveSpaceForScale) {
        auto label{
            getGridPointLabel(options.physicalOrigin.p.x() +
                                  static_cast<double>(srcX) * xPixelWidth,
                              srcX == 0, options.lengthUnits)};
        // paint text label & extend tick mark if there is enough space
        if (int labelWidth{p.fontMetrics().horizontalAdvance(label)};
            prevTextEnd + (labelWidth + 1) / 2 + 4 < x &&
            x + (labelWidth + 1) / 2 <= rendered.pixmapImageSize.width()) {
          p.drawText(
              QRect(x + rendered.offset.x() - labelWidth / 2,
                    rendered.pixmapImageSize.height() - 1 + options.tickLength,
                    labelWidth, rendered.offset.y() - options.tickLength),
              Qt::AlignHCenter | Qt::AlignTop, label);
          p.drawLine(
              x + rendered.offset.x(), rendered.pixmapImageSize.height() - 1,
              x + rendered.offset.x(),
              rendered.pixmapImageSize.height() - 1 + options.tickLength);
          prevTextEnd = x + (labelWidth + 1) / 2;
        }
      }
    }
    auto yGridEdgePixels{
        getGridEdgePixels(srcImage.height(), gridSpacing.pixelStep)};
    if (options.drawGrid) {
      for (int srcY : yGridEdgePixels) {
        int y{sourceYToPixmapY(srcY, rendered.pixmapImageSize.height(),
                               srcImage.height(), options.flipYAxis)};
        p.drawLine(rendered.offset.x(), y,
                   rendered.offset.x() + rendered.pixmapImageSize.width() - 1,
                   y);
      }
    }
    auto yScaleEdgePixels{
        getScaleEdgePixels(srcImage.height(), gridSpacing.pixelStep)};
    double yPixelWidth{options.physicalSize.height() /
                       static_cast<double>(srcImage.height())};
    for (int srcY : yScaleEdgePixels) {
      int y{sourceYToPixmapY(srcY, rendered.pixmapImageSize.height(),
                             srcImage.height(), options.flipYAxis)};
      if (options.drawScale && haveSpaceForScale) {
        auto label{
            getGridPointLabel(options.physicalOrigin.p.y() +
                                  static_cast<double>(srcY) * yPixelWidth,
                              srcY == 0, options.lengthUnits)};
        p.drawText(QRect(0, y - rendered.offset.y() / 2,
                         rendered.offset.x() - options.tickLength,
                         rendered.offset.y()),
                   Qt::AlignVCenter | Qt::AlignRight, label);
        p.drawLine(rendered.offset.x() - options.tickLength, y,
                   rendered.offset.x(), y);
      }
    }
  }
  if (options.verticalIndicatorSourceX >= 0 &&
      options.verticalIndicatorSourceX < srcImage.width()) {
    QPen indicatorPen(Qt::black);
    indicatorPen.setCosmetic(true);
    p.setPen(indicatorPen);
    int x{std::clamp(
        static_cast<int>(std::lround(
            (static_cast<double>(options.verticalIndicatorSourceX) + 0.5) *
            static_cast<double>(rendered.pixmapImageSize.width()) /
            static_cast<double>(srcImage.width()))) -
            1,
        0, rendered.pixmapImageSize.width() - 1)};
    p.drawLine(rendered.offset.x() + x, 0, rendered.offset.x() + x,
               rendered.pixmapImageSize.height() - 1);
  }
  p.end();
  return rendered;
}

} // namespace sme::gui
