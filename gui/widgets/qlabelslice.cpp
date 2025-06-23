#include "qlabelslice.hpp"
#include "sme/logger.hpp"

QLabelSlice::QLabelSlice(QWidget *parent) : QLabel(parent) {
  setMouseTracking(true);
  setAlignment(Qt::AlignmentFlag::AlignTop | Qt::AlignmentFlag::AlignLeft);
  setMinimumSize(1, 1);
  setWordWrap(true);
}

void QLabelSlice::setImage(const QImage &img, bool invertYAxis) {
  flipYAxis = invertYAxis;
  slicePixels.clear();
  slicePixels.reserve(static_cast<std::size_t>(imgOriginal.width()) +
                      static_cast<std::size_t>(imgOriginal.height()));
  imgOriginal = img.convertToFormat(QImage::Format_RGB32);
  if (flipYAxis) {
    imgOriginal = imgOriginal.flipped(Qt::Orientation::Vertical);
  }
  imgSliced = QImage(imgOriginal.size(), QImage::Format_ARGB32);
  fadeOriginalImage();
  // on loading new image, set a minimum size for the widget
  constexpr int minImageWidth{100};
  if (this->width() < minImageWidth) {
    this->resize(minImageWidth, minImageWidth);
  }
}

const QImage &QLabelSlice::getImage() const { return imgSliced; }

void QLabelSlice::setAspectRatioMode(Qt::AspectRatioMode mode) {
  aspectRatioMode = mode;
}
void QLabelSlice::setTransformationMode(Qt::TransformationMode mode) {
  transformationMode = mode;
}

void QLabelSlice::setSlice(const QPoint &start, const QPoint &end) {
  slicePixels.clear();
  fadeOriginalImage();
  QPoint dp{end - start};
  int norm = std::max(std::abs(dp.x()), std::abs(dp.y()));
  QPoint diff{0, 0};
  int np = norm + 1;
  norm = std::max(norm, 1); // avoid dividing by zero if start == end
  for (int i = 0; i < np; ++i) {
    QPoint pi(start.x() + diff.x() / norm, start.y() + diff.y() / norm);
    slicePixels.push_back(pi);
    if (flipYAxis) {
      pi.setY(imgSliced.height() - 1 - pi.y());
    }
    auto c{imgOriginal.pixel(pi)};
    imgSliced.setPixel(pi, c);
    diff += dp;
  }
  resizeImage(this->size());
}

const std::vector<QPoint> &QLabelSlice::getSlicePixels() const {
  return slicePixels;
}

void QLabelSlice::setHorizontalSlice(int y) {
  setSlice(QPoint(0, y), QPoint(imgOriginal.width() - 1, y));
}

void QLabelSlice::setVerticalSlice(int x) {
  setSlice(QPoint(x, imgOriginal.height() - 1), QPoint(x, 0));
}

void QLabelSlice::mousePressEvent(QMouseEvent *ev) {
  if (ev->buttons() != Qt::NoButton && setPixel(ev, startPixel)) {
    currentPixel = startPixel;
    mouseIsDown = true;
    emit mouseDown(currentPixel);
  }
}

void QLabelSlice::mouseMoveEvent(QMouseEvent *ev) {
  if (!setPixel(ev, currentPixel)) {
    return;
  }
  emit mouseOver(currentPixel);
  if (mouseIsDown) {
    SPDLOG_TRACE("mouseDown ({},{})", currentPixel.x(), currentPixel.y());
    emit mouseDown(currentPixel);
  }
}

void QLabelSlice::mouseReleaseEvent(QMouseEvent *ev) {
  if (mouseIsDown) {
    SPDLOG_TRACE("sliceDrawn ({},{})->({},{})", startPixel.x(), startPixel.y(),
                 currentPixel.x(), currentPixel.y());
    emit sliceDrawn(startPixel, currentPixel);
  }
  ev->accept();
  mouseIsDown = false;
}

void QLabelSlice::mouseDoubleClickEvent(QMouseEvent *ev) { ev->accept(); }

void QLabelSlice::wheelEvent(QWheelEvent *ev) {
  emit mouseWheelEvent(ev->angleDelta().y());
}

void QLabelSlice::resizeEvent(QResizeEvent *event) {
  if (event->oldSize() != event->size()) {
    resizeImage(event->size());
  }
}

bool QLabelSlice::setPixel(const QMouseEvent *ev, QPoint &pixel) {
  int x{ev->pos().x()};
  int y{ev->pos().y()};
  if (imgSliced.isNull() || pixmap.isNull() || (x >= pixmap.width()) ||
      (y >= pixmap.height()) || (x < 0) || (y < 0)) {
    return false;
  }
  pixel.setX((imgSliced.width() * x) / pixmap.width());
  pixel.setY((imgSliced.height() * y) / pixmap.height());
  if (flipYAxis) {
    pixel.setY(imgSliced.height() - 1 - pixel.y());
  }
  return true;
}

void QLabelSlice::fadeOriginalImage() {
  constexpr int transparencyAlpha{50};
  for (int x = 0; x < imgOriginal.width(); ++x) {
    for (int y = 0; y < imgOriginal.height(); ++y) {
      auto c{imgOriginal.pixel(x, y)};
      auto cFaded{qRgba(qRed(c), qGreen(c), qBlue(c), transparencyAlpha)};
      imgSliced.setPixel(x, y, cFaded);
    }
  }
}

void QLabelSlice::resizeImage(const QSize &size) {
  if (imgSliced.isNull()) {
    this->clear();
    return;
  }
  pixmap = QPixmap::fromImage(
      imgSliced.scaled(size, aspectRatioMode, transformationMode));
  SPDLOG_DEBUG("resize -> {}x{}, pixmap -> {}x{}", size.width(), size.height(),
               pixmap.size().width(), pixmap.size().height());
  this->setPixmap(pixmap);
}
