#include "qlabelmousetracker.hpp"

#include "logger.hpp"

QLabelMouseTracker::QLabelMouseTracker(QWidget *parent) : QLabel(parent) {
  setMouseTracking(true);
  setAlignment(Qt::AlignmentFlag::AlignTop | Qt::AlignmentFlag::AlignLeft);
  setMinimumSize(1, 1);
  setWordWrap(true);
}

void QLabelMouseTracker::setImage(const QImage &img) {
  image = img;
  constexpr int minImageWidth{100};
  // on loading new image, set a minimum size for the widget
  if (this->width() < minImageWidth) {
    this->resize(minImageWidth, minImageWidth);
  }
  resizeImage(this->size());
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

void QLabelMouseTracker::mousePressEvent(QMouseEvent *ev) {
  if (ev->buttons() != Qt::NoButton) {
    if (setCurrentPixel(ev)) {
      // update current colour and emit mouseClicked signal
      colour = image.pixelColor(currentPixel).rgb();
      SPDLOG_DEBUG("pixel ({},{}) -> colour {:x}", currentPixel.x(),
                   currentPixel.y(), colour);
      if (maskImage.valid(currentPixel)) {
        maskIndex = static_cast<int>(maskImage.pixel(currentPixel) & RGB_MASK);
      }
      emit mouseClicked(colour, currentPixel);
    }
  }
}

void QLabelMouseTracker::mouseMoveEvent(QMouseEvent *ev) {
  if (setCurrentPixel(ev)) {
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

bool QLabelMouseTracker::setCurrentPixel(const QMouseEvent *ev) {
  if (image.isNull() || pixmap.isNull() || (ev->pos().x() >= pixmap.width()) ||
      (ev->pos().y() >= pixmap.height()) || ev->pos().x() < 0 ||
      ev->pos().y() < 0) {
    return false;
  }
  currentPixel.setX((image.width() * ev->pos().x()) / pixmap.width());
  currentPixel.setY((image.height() * ev->pos().y()) / pixmap.height());
  SPDLOG_TRACE("mouse at ({},{}) -> pixel ({},{})", ev->pos().x(),
               ev->pos().y(), currentPixel.x(), currentPixel.y());
  return true;
}

void QLabelMouseTracker::resizeImage(const QSize &size) {
  if (image.isNull()) {
    this->clear();
    return;
  }
  pixmap = QPixmap::fromImage(
      image.scaled(size, aspectRatioMode, transformationMode));
  SPDLOG_DEBUG("resize -> {}x{}, pixmap -> {}x{}", size.width(), size.height(),
               pixmap.size().width(), pixmap.size().height());
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
