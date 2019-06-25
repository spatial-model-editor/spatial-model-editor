#include "qlabelmousetracker.h"
#include <QDebug>

QLabelMouseTracker::QLabelMouseTracker(QWidget *parent) : QLabel(parent) {
  setMouseTracking(true);
}

void QLabelMouseTracker::setImage(const QImage &img) {
  image = img;
  pixmap = QPixmap::fromImage(image);
  this->setPixmap(pixmap.scaledToWidth(this->width(), Qt::FastTransformation));

  // set first mouse click to top left corner of image
  currentPixel.setX(0);
  currentPixel.setY(0);
  colour = image.pixelColor(currentPixel).rgb();
  emit mouseClicked(colour);
}

const QImage &QLabelMouseTracker::getImage() const { return image; }

const QRgb &QLabelMouseTracker::getColour() const { return colour; }

void QLabelMouseTracker::mouseMoveEvent(QMouseEvent *ev) {
  // on mouse move: update current pixel
  currentPixel.setX((pixmap.width() * ev->pos().x()) / this->size().width());
  currentPixel.setY((pixmap.height() * ev->pos().y()) / this->size().height());
  qDebug("QLabelMouseTracker :: mouseMove at (%d,%d) -> pixel (%d,%d)",
         ev->pos().x(), ev->pos().y(), currentPixel.x(), currentPixel.y());
  // possible todo: highlight colour region on mouseover?
}

void QLabelMouseTracker::mousePressEvent(QMouseEvent *ev) {
  if (ev->buttons() != Qt::NoButton) {
    // on mouse click: update current colour and emit mouseClicked signal
    if (!pixmap.isNull()) {
      colour = image.pixelColor(currentPixel).rgb();
      qDebug(
          "QLabelMouseTracker :: mousePress at (%d,%d) -> pixel (%d,%d) with "
          "colour %u",
          ev->pos().x(), ev->pos().y(), currentPixel.x(), currentPixel.y(),
          colour);
      emit mouseClicked(colour);
    }
  }
}

void QLabelMouseTracker::resizeEvent(QResizeEvent *event) {
  if (!pixmap.isNull()) {
    this->setPixmap(
        pixmap.scaledToWidth(event->size().width(), Qt::FastTransformation));
  }
}
