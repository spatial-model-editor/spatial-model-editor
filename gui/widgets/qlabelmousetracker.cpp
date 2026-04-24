#include "qlabelmousetracker.hpp"
#include "imagerenderutils.hpp"
#include "sme/logger.hpp"
#include <QPainter>
#include <algorithm>
#include <cmath>
#include <vector>

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

void QLabelMouseTracker::resizeImage(const QSize &size) {
  if (image.empty()) {
    this->clear();
    return;
  }
  const auto &srcImage{image[currentVoxel.z]};
  sme::gui::ImageRenderOptions opts;
  opts.aspectRatioMode = aspectRatioMode;
  opts.transformationMode = transformationMode;
  opts.drawGrid = drawGrid;
  opts.drawScale = drawScale;
  opts.flipYAxis = flipYAxis;
  opts.verticalIndicatorSourceX = verticalIndicatorSourceX;
  opts.tickLength = tickLength;
  opts.physicalOrigin = physicalOrigin;
  opts.physicalSize = physicalSize;
  opts.lengthUnits = lengthUnits;
  auto rendered{sme::gui::renderImageWithOverlays(srcImage, size, opts)};
  pixmap = rendered.pixmap;
  pixmapImageSize = rendered.pixmapImageSize;
  offset = rendered.offset;
  SPDLOG_DEBUG("resize -> {}x{}, pixmap -> {}x{}, image -> {}x{}", size.width(),
               size.height(), pixmap.width(), pixmap.height(),
               pixmapImageSize.width(), pixmapImageSize.height());
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

void QLabelMouseTracker::setVerticalIndicatorSourceX(int sourceX) {
  if (verticalIndicatorSourceX == sourceX) {
    return;
  }
  verticalIndicatorSourceX = sourceX;
  resizeImage(size());
}

void QLabelMouseTracker::setPhysicalUnits(const QString &units) {
  lengthUnits = units;
}

void QLabelMouseTracker::setPhysicalOrigin(const sme::common::VoxelF &origin) {
  physicalOrigin = origin;
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
  image.flipYAxis();
  resizeImage(this->size());
}
