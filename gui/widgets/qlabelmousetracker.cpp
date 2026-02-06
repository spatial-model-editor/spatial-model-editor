#include "qlabelmousetracker.hpp"
#include "sme/logger.hpp"
#include <QPainter>
#include <cmath>

QLabelMouseTracker::QLabelMouseTracker(QWidget *parent) : QLabel(parent) {
  setMouseTracking(true);
  setAlignment(Qt::AlignmentFlag::AlignTop | Qt::AlignmentFlag::AlignLeft);
  setMinimumSize(1, 1);
  setWordWrap(true);
}

void QLabelMouseTracker::setImage(const sme::common::ImageStack &img) {
  image = img;
  if (currentVoxel.z >= image.volume().depth()) {
    currentVoxel.z = 0;
    if (zSlider != nullptr) {
      zSlider->setValue(0);
    }
  }
  if (flipYAxis) {
    image.flipYAxis();
  }
  constexpr int minImageWidth{100};
  // on loading new image, set a minimum volume for the widget
  if (this->width() < minImageWidth) {
    this->resize(minImageWidth, minImageWidth);
  }
  if (zSlider != nullptr) {
    zSlider->setEnabled(true);
    zSlider->setMinimum(0);
    zSlider->setMaximum(static_cast<int>(image.volume().depth()) - 1);
  }
  physicalSize = image.voxelSize() * image.volume();
  resizeImage(this->size());
  setCurrentPixel(mapFromGlobal(QCursor::pos()));
}

const sme::common::ImageStack &QLabelMouseTracker::getImage() const {
  return image;
}

void QLabelMouseTracker::setZSlider(QSlider *slider) {
  zSlider = slider;
  if (zSlider == nullptr) {
    return;
  }
  connect(zSlider, &QSlider::valueChanged, this,
          &QLabelMouseTracker::setZIndex);
}

QSlider *QLabelMouseTracker::getZSlider() const { return zSlider; }

void QLabelMouseTracker::setZIndex(int value) {
  auto z{static_cast<std::size_t>(value)};
  if (z >= image.volume().depth() || z == currentVoxel.z) {
    return;
  }
  currentVoxel.z = z;
  resizeImage(this->size());
}

void QLabelMouseTracker::setMaskImage(const sme::common::ImageStack &img) {
  maskImage = img;
}

const sme::common::ImageStack &QLabelMouseTracker::getMaskImage() const {
  return maskImage;
}

void QLabelMouseTracker::setImages(
    const std::pair<sme::common::ImageStack, sme::common::ImageStack>
        &imgPair) {
  setImage(imgPair.first);
  setMaskImage(imgPair.second);
}

QRgb QLabelMouseTracker::getColor() const { return color; }

int QLabelMouseTracker::getMaskIndex() const { return maskIndex; }

QPointF QLabelMouseTracker::getRelativePosition() const {
  if (image.empty() || pixmap.isNull() || pixmap.width() == 0 ||
      pixmap.height() == 0) {
    return {};
  }
  auto xRelPos{static_cast<double>(currentVoxel.p.x()) /
               static_cast<double>(image[currentVoxel.z].width())};
  auto yRelPos{static_cast<double>(currentVoxel.p.y()) /
               static_cast<double>(image[currentVoxel.z].height())};
  auto xAspectRatioFactor{static_cast<double>(pixmapImageSize.width()) /
                          static_cast<double>(pixmap.width())};
  auto yAspectRatioFactor{static_cast<double>(pixmapImageSize.height()) /
                          static_cast<double>(pixmap.height())};
  return {xRelPos * xAspectRatioFactor, yRelPos * yAspectRatioFactor};
}

void QLabelMouseTracker::mousePressEvent(QMouseEvent *ev) {
  if (ev->buttons() == Qt::NoButton) {
    return;
  }
  if (setCurrentPixel(ev->pos())) {
    // update current color and emit mouseClicked signal
    auto imagePixel{currentVoxel};
    if (flipYAxis) {
      imagePixel.p.setY(image[currentVoxel.z].height() - 1 - imagePixel.p.y());
    }
    color = image[currentVoxel.z].pixelColor(imagePixel.p).rgb();
    SPDLOG_DEBUG("imagePixel ({},{},{}) -> color {:x}", imagePixel.p.x(),
                 imagePixel.p.y(), imagePixel.z, color);
    if (!maskImage.empty() && maskImage[currentVoxel.z].valid(currentVoxel.p)) {
      maskIndex = static_cast<int>(
          maskImage[currentVoxel.z].pixel(currentVoxel.p) & RGB_MASK);
    }
    emit mouseClicked(color, currentVoxel);
  }
}

void QLabelMouseTracker::mouseMoveEvent(QMouseEvent *ev) {
  if (setCurrentPixel(ev->pos())) {
    emit mouseOver(currentVoxel);
  }
}

void QLabelMouseTracker::mouseReleaseEvent(QMouseEvent *ev) { ev->accept(); }

void QLabelMouseTracker::mouseDoubleClickEvent(QMouseEvent *ev) {
  ev->accept();
}

void QLabelMouseTracker::wheelEvent(QWheelEvent *ev) {
  emit mouseWheelEvent(ev);
}

void QLabelMouseTracker::resizeEvent(QResizeEvent *event) {
  if (event->oldSize() != event->size()) {
    resizeImage(event->size());
  }
}

bool QLabelMouseTracker::setCurrentPixel(const QPoint &pos) {
  if (image.empty() || pixmap.isNull() ||
      (pos.x() >= pixmapImageSize.width() + offset.x()) ||
      (pos.y() >= pixmapImageSize.height()) || pos.x() < offset.x() ||
      pos.y() < 0) {
    return false;
  }
  currentVoxel.p.setX((image[currentVoxel.z].width() * (pos.x() - offset.x())) /
                      pixmapImageSize.width());
  currentVoxel.p.setY((image[currentVoxel.z].height() * pos.y()) /
                      pixmapImageSize.height());
  if (flipYAxis) {
    currentVoxel.p.setY(image[currentVoxel.z].height() - currentVoxel.p.y() -
                        1);
  }
  SPDLOG_TRACE("mouse at ({},{}) -> voxel ({},{},{})", pos.x(), pos.y(),
               currentVoxel.p.x(), currentVoxel.p.y(), currentVoxel.z);
  return true;
}

struct GridSpacing {
  double physicalWidth{0.0};
  int pixelStep{1};
};

static GridSpacing getGridWidth(const sme::common::VolumeF &physicalSize,
                                const QSize &pixmapSize,
                                const QSize &imagePixelSize) {
  constexpr double minWidthPixels{20};
  // start with grid of width 1 physical unit
  double gridPixelWidth{static_cast<double>(pixmapSize.width())};
  double gridPhysicalWidth{physicalSize.width()};
  // rescale with in pixels to: ~ [1, 2] * minWidthPixels
  // hopefully with reasonably nice looking numerical intervals
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

static QString getGridPointLabel(int i, double gridPhysicalWidth,
                                 const QString &lengthUnits) {
  double value{static_cast<double>(i) * gridPhysicalWidth};
  auto label{QString::number(value, 'g', 3)};
  if (i == 0) {
    label.append(" ").append(lengthUnits);
  }
  return label;
}

static QSize getPixmapSize(const QSize &displaySize,
                           double physicalAspectRatio) {
  double displayAspectRatio{static_cast<double>(displaySize.width()) /
                            static_cast<double>(displaySize.height())};
  if (displayAspectRatio > physicalAspectRatio) {
    return {std::max(1, static_cast<int>(displaySize.height() *
                                         physicalAspectRatio)),
            displaySize.height()};
  }
  return {
      displaySize.width(),
      std::max(1, static_cast<int>(displaySize.width() / physicalAspectRatio)),
  };
}

void QLabelMouseTracker::resizeImage(const QSize &size) {
  if (image.empty()) {
    this->clear();
    return;
  }
  pixmap = QPixmap(size);
  pixmap.fill(QColor(0, 0, 0, 0));
  offset = {0, 0};
  QPainter p(&pixmap);
  bool haveSpaceForScale{false};
  if (drawScale) {
    int w{p.fontMetrics().horizontalAdvance("8e+88")};
    int h{p.fontMetrics().height()};
    // only draw scale if picture is bigger than labels
    if (size.width() > 2 * w && size.height() > 2 * h) {
      offset = {w + tickLength, h + tickLength};
      haveSpaceForScale = true;
    }
  }
  QSize availableSize{size};
  availableSize.rwidth() -= offset.x();
  availableSize.rheight() -= offset.y();
  pixmapImageSize = getPixmapSize(availableSize,
                                  physicalSize.width() / physicalSize.height());
  const auto &srcImage{image[currentVoxel.z]};
  double sx{static_cast<double>(pixmapImageSize.width()) /
            static_cast<double>(srcImage.width())};
  double sy{static_cast<double>(pixmapImageSize.height()) /
            static_cast<double>(srcImage.height())};
  p.setRenderHint(QPainter::SmoothPixmapTransform,
                  transformationMode == Qt::SmoothTransformation);
  p.save();
  p.translate(offset.x(), 0);
  p.scale(sx, sy);
  p.drawImage(QPoint(0, 0), srcImage);
  p.restore();
  SPDLOG_DEBUG("resize -> {}x{}, pixmap -> {}x{}, image -> {}x{}", size.width(),
               size.height(), pixmap.width(), pixmap.height(),
               pixmapImageSize.width(), pixmapImageSize.height());
  if (drawGrid || (drawScale && haveSpaceForScale)) {
    QPen gridPen(QColor(127, 127, 127));
    gridPen.setCosmetic(true);
    p.setPen(gridPen);
    auto gridSpacing =
        getGridWidth(physicalSize, pixmapImageSize, srcImage.size());
    int prevTextEnd{-1000};
    int maxXIndex{(srcImage.width() - 1) / gridSpacing.pixelStep};
    if (drawGrid) {
      p.save();
      p.translate(offset.x(), 0);
      p.scale(sx, sy);
      for (int i = 0; i <= maxXIndex; ++i) {
        int srcX{i * gridSpacing.pixelStep};
        p.drawLine(QPointF(srcX, 0), QPointF(srcX, srcImage.height()));
      }
      p.restore();
    }
    for (int i = 0; i <= maxXIndex; ++i) {
      int srcX{i * gridSpacing.pixelStep};
      int x{static_cast<int>(std::lround(srcX * sx))};
      if (drawScale && haveSpaceForScale) {
        auto label{
            getGridPointLabel(i, gridSpacing.physicalWidth, lengthUnits)};
        // paint text label & extend tick mark if there is enough space
        if (int labelWidth{p.fontMetrics().horizontalAdvance(label)};
            prevTextEnd + (labelWidth + 1) / 2 + 4 < x &&
            x + (labelWidth + 1) / 2 <= pixmapImageSize.width()) {
          p.drawText(QRect(x + offset.x() - labelWidth / 2,
                           pixmapImageSize.height() - 1 + tickLength,
                           labelWidth, offset.y() - tickLength),
                     Qt::AlignHCenter | Qt::AlignTop, label);
          p.drawLine(x + offset.x(), pixmapImageSize.height() - 1,
                     x + offset.x(), pixmapImageSize.height() - 1 + tickLength);
          prevTextEnd = x + (labelWidth + 1) / 2;
        }
      }
    }
    int maxYIndex{(srcImage.height() - 1) / gridSpacing.pixelStep};
    if (drawGrid) {
      p.save();
      p.translate(offset.x(), 0);
      p.scale(sx, sy);
      for (int i = 0; i <= maxYIndex; ++i) {
        int srcY{i * gridSpacing.pixelStep};
        if (!flipYAxis) {
          srcY = srcImage.height() - 1 - srcY;
        }
        p.drawLine(QPointF(0, srcY), QPointF(srcImage.width(), srcY));
      }
      p.restore();
    }
    for (int i = 0; i <= maxYIndex; ++i) {
      int srcY{i * gridSpacing.pixelStep};
      auto y{static_cast<int>(std::lround(srcY * sy))};
      if (!flipYAxis) {
        y = pixmapImageSize.height() - 1 - y;
      }
      if (drawScale && haveSpaceForScale) {
        auto label{
            getGridPointLabel(i, gridSpacing.physicalWidth, lengthUnits)};
        p.drawText(
            QRect(0, y - offset.y() / 2, offset.x() - tickLength, offset.y()),
            Qt::AlignVCenter | Qt::AlignRight, label);
        p.drawLine(offset.x() - tickLength, y, offset.x(), y);
      }
    }
  }
  p.end();
  this->setPixmap(pixmap);
}

void QLabelMouseTracker::setAspectRatioMode(Qt::AspectRatioMode mode) {
  aspectRatioMode = mode;
  resizeImage(size());
}

void QLabelMouseTracker::setTransformationMode(Qt::TransformationMode mode) {
  transformationMode = mode;
  resizeImage(size());
}

void QLabelMouseTracker::setPhysicalUnits(const QString &units) {
  lengthUnits = units;
}

void QLabelMouseTracker::displayGrid(bool enable) {
  drawGrid = enable;
  resizeImage(this->size());
}

void QLabelMouseTracker::displayScale(bool enable) {
  drawScale = enable;
  resizeImage(this->size());
}

void QLabelMouseTracker::invertYAxis(bool enable) {
  if (enable == flipYAxis) {
    return;
  }
  flipYAxis = enable;
  image.flipYAxis();
  resizeImage(this->size());
}
