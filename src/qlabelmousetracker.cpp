#include "qlabelmousetracker.hpp"

#include "logger.hpp"

QLabelMouseTracker::QLabelMouseTracker(QWidget *parent) : QLabel(parent) {
  setMouseTracking(true);
  setAlignment(Qt::AlignmentFlag::AlignTop | Qt::AlignmentFlag::AlignLeft);
  setMinimumSize(1, 1);
  setWordWrap(true);
  pixmap.fill();
}

void QLabelMouseTracker::setImage(const QImage &img) {
  image = img;
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

void QLabelMouseTracker::mouseMoveEvent(QMouseEvent *ev) {
  setCurrentPixel(ev);
  emit mouseOver(currentPixel);
}

void QLabelMouseTracker::mousePressEvent(QMouseEvent *ev) {
  if (ev->buttons() != Qt::NoButton) {
    setCurrentPixel(ev);
    if (!pixmap.isNull()) {
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

void QLabelMouseTracker::resizeEvent(QResizeEvent *event) {
  resizeImage(event->size());
}

void QLabelMouseTracker::wheelEvent(QWheelEvent *ev) {
  emit mouseWheelEvent(ev);
}

void QLabelMouseTracker::setCurrentPixel(QMouseEvent *ev) {
  if (!image.isNull() && !pixmap.isNull()) {
    currentPixel.setX((image.width() * ev->pos().x()) / pixmap.width());
    currentPixel.setY((image.height() * ev->pos().y()) / pixmap.height());
    SPDLOG_TRACE("mouse at ({},{}) -> pixel ({},{})", ev->pos().x(),
                 ev->pos().y(), currentPixel.x(), currentPixel.y());
  }
}

void QLabelMouseTracker::resizeImage(const QSize &size) {
  if (image.isNull()) {
    pixmap = {};
    this->setPixmap({});
    return;
  }
  pixmap = QPixmap::fromImage(
      image.scaled(size, Qt::KeepAspectRatio, Qt::FastTransformation));
  SPDLOG_DEBUG("resize -> {}x{}, pixmap -> {}x{}", size.width(), size.height(),
               pixmap.size().width(), pixmap.size().height());
  this->setPixmap(pixmap);
}
