#include "qlabelmousetracker.hpp"
#include "sme/logger.hpp"
#include <QPainter>

QLabelMouseTracker::QLabelMouseTracker(QWidget *parent) : QLabel(parent) {
  setMouseTracking(true);
  setAlignment(Qt::AlignmentFlag::AlignTop | Qt::AlignmentFlag::AlignLeft);
  setMinimumSize(1, 1);
  setWordWrap(true);
}

void QLabelMouseTracker::setImage(const QImage &img) {
  image = img;
  if (flipYAxis) {
    image = image.mirrored();
  }
  constexpr int minImageWidth{100};
  // on loading new image, set a minimum size for the widget
  if (this->width() < minImageWidth) {
    this->resize(minImageWidth, minImageWidth);
  }
  resizeImage(this->size());
  setCurrentPixel(mapFromGlobal(QCursor::pos()));
}

const QImage &QLabelMouseTracker::getImage() const { return image; }

void QLabelMouseTracker::setMaskImage(const QImage &img) { maskImage = img; }

const QImage &QLabelMouseTracker::getMaskImage() const { return maskImage; }

void QLabelMouseTracker::setImages(const std::pair<QImage, QImage> &imgPair) {
  setImage(imgPair.first);
  setMaskImage(imgPair.second);
}

const QRgb &QLabelMouseTracker::getColour() const { return colour; }

int QLabelMouseTracker::getMaskIndex() const { return maskIndex; }

QPointF QLabelMouseTracker::getRelativePosition() const {
  auto xRelPos{static_cast<double>(currentPixel.x()) /
               static_cast<double>(image.width())};
  auto yRelPos{static_cast<double>(currentPixel.y()) /
               static_cast<double>(image.height())};
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
    // update current colour and emit mouseClicked signal
    auto imagePixel{currentPixel};
    if (flipYAxis) {
      imagePixel.setY(image.height() - 1 - imagePixel.y());
    }
    colour = image.pixelColor(imagePixel).rgb();
    SPDLOG_DEBUG("imagePixel ({},{}) -> colour {:x}", imagePixel.x(),
                 imagePixel.y(), colour);
    if (maskImage.valid(currentPixel)) {
      maskIndex = static_cast<int>(maskImage.pixel(currentPixel) & RGB_MASK);
    }
    emit mouseClicked(colour, currentPixel);
  }
}

void QLabelMouseTracker::mouseMoveEvent(QMouseEvent *ev) {
  if (setCurrentPixel(ev->pos())) {
    emit mouseOver(currentPixel);
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
  if (image.isNull() || pixmap.isNull() ||
      (pos.x() >= pixmapImageSize.width() + offset.x()) ||
      (pos.y() >= pixmapImageSize.height()) || pos.x() < offset.x() ||
      pos.y() < 0) {
    return false;
  }
  currentPixel.setX((image.width() * (pos.x() - offset.x())) /
                    pixmapImageSize.width());
  currentPixel.setY((image.height() * pos.y()) / pixmapImageSize.height());
  if (flipYAxis) {
    currentPixel.setY(image.height() - currentPixel.y() - 1);
  }
  SPDLOG_TRACE("mouse at ({},{}) -> pixel ({},{})", pos.x(), pos.y(),
               currentPixel.x(), currentPixel.y());
  return true;
}

static std::pair<double, double> getGridWidth(const QSizeF &physicalSize,
                                              const QSize &imageSize) {
  constexpr double minWidthPixels{20};
  // start with grid of width 1 physical unit
  double gridPixelWidth{static_cast<double>(imageSize.width()) /
                        physicalSize.width()};
  double gridPhysicalWidth{1.0};
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
  return {gridPhysicalWidth, gridPixelWidth};
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

static QSize getActualImageSize(const QSize &before, const QSize &after) {
  QSize sz{after};
  if (after.width() * before.height() > after.height() * before.width()) {
    sz.rwidth() = before.width() * after.height() / before.height();
  } else {
    sz.rheight() = before.height() * after.width() / before.width();
  }
  return sz;
}

void QLabelMouseTracker::resizeImage(const QSize &size) {
  if (image.isNull()) {
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
  auto scaledImage{
      image.scaled(availableSize, aspectRatioMode, transformationMode)};
  pixmapImageSize = getActualImageSize(image.size(), scaledImage.size());
  p.drawImage(QPoint(offset.x(), 0), scaledImage);
  SPDLOG_DEBUG("resize -> {}x{}, pixmap -> {}x{}, image -> {}x{}", size.width(),
               size.height(), pixmap.width(), pixmap.height(),
               pixmapImageSize.width(), pixmapImageSize.height());
  if (drawGrid || (drawScale && haveSpaceForScale)) {
    p.setPen(QColor(127, 127, 127));
    auto [gridPhysicalWidth, gridPixelWidth] =
        getGridWidth(physicalSize, pixmapImageSize);
    int prevTextEnd{-1000};
    for (int i = 0; i < (pixmapImageSize.width() - 1) / gridPixelWidth; ++i) {
      int x{static_cast<int>(gridPixelWidth * static_cast<double>(i))};
      if (drawGrid) {
        p.drawLine(x + offset.x(), 0, x + offset.x(), pixmapImageSize.height());
      }
      if (drawScale && haveSpaceForScale) {
        auto label{getGridPointLabel(i, gridPhysicalWidth, lengthUnits)};
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
    for (int i = 0;
         i < pixmapImageSize.height() / static_cast<int>(gridPixelWidth); ++i) {
      auto y{static_cast<int>(gridPixelWidth * static_cast<double>(i))};
      if (!flipYAxis) {
        y = pixmapImageSize.height() - 1 - y;
      }
      if (drawGrid) {
        p.drawLine(offset.x(), y, pixmapImageSize.width() + offset.x(), y);
      }
      if (drawScale && haveSpaceForScale) {
        auto label{getGridPointLabel(i, gridPhysicalWidth, lengthUnits)};
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

void QLabelMouseTracker::setPhysicalSize(const QSizeF &size,
                                         const QString &units) {
  physicalSize = size;
  lengthUnits = units;
  resizeImage(this->size());
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
  image = image.mirrored();
  resizeImage(this->size());
}
