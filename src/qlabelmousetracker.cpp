#include "qlabelmousetracker.hpp"

#include "logger.hpp"

QLabelMouseTracker::QLabelMouseTracker(QWidget *parent) : QLabel(parent) {
  setMouseTracking(true);
  setAlignment(Qt::AlignmentFlag::AlignTop | Qt::AlignmentFlag::AlignLeft);
  setMinimumSize(1, 1);
  pixmap.fill();
}

void QLabelMouseTracker::setImage(const QImage &img) {
  image = img;
  resizeImage(this->size());
  // set first mouse click to top left corner of image
  currentPixel.setX(0);
  currentPixel.setY(0);
  colour = image.pixelColor(currentPixel).rgb();
  emit mouseClicked(colour, currentPixel);
}

const QImage &QLabelMouseTracker::getImage() const { return image; }

const QRgb &QLabelMouseTracker::getColour() const { return colour; }

void QLabelMouseTracker::mouseMoveEvent(QMouseEvent *ev) {
  setCurrentPixel(ev);
}

void QLabelMouseTracker::mousePressEvent(QMouseEvent *ev) {
  if (ev->buttons() != Qt::NoButton) {
    setCurrentPixel(ev);
    // update current colour and emit mouseClicked signal
    if (!pixmap.isNull()) {
      colour = image.pixelColor(currentPixel).rgb();
      SPDLOG_DEBUG("pixel ({},{}) -> colour {:x}", currentPixel.x(),
                   currentPixel.y(), colour);
      emit mouseClicked(colour, currentPixel);
    }
  }
}

void QLabelMouseTracker::resizeEvent(QResizeEvent *event) {
  resizeImage(event->size());
}

void QLabelMouseTracker::setCurrentPixel(QMouseEvent *ev) {
  currentPixel.setX((image.width() * ev->pos().x()) / pixmap.width());
  currentPixel.setY((image.height() * ev->pos().y()) / pixmap.height());
  SPDLOG_DEBUG("mouse at ({},{}) -> pixel ({},{})", ev->pos().x(),
               ev->pos().y(), currentPixel.x(), currentPixel.y());
}

void QLabelMouseTracker::resizeImage(const QSize &size) {
  if (!image.isNull()) {
    pixmap = QPixmap::fromImage(
        image.scaled(size, Qt::KeepAspectRatio, Qt::FastTransformation));
    SPDLOG_DEBUG("resize -> {}x{}, pixmap -> {}x{}", size.width(),
                 size.height(), pixmap.size().width(), pixmap.size().height());
    this->setPixmap(pixmap);
  }
}
