#include "qlabelmousetracker.h"
#include <QDebug>

QLabelMouseTracker::QLabelMouseTracker(QWidget *parent) : QLabel(parent) {}

void QLabelMouseTracker::setImage(const QImage &img) {
  image = img;
  pixmap = QPixmap::fromImage(image);
  this->setPixmap(pixmap.scaledToWidth(this->width(), Qt::FastTransformation));

  // set first mouse click to top left corner of image
  currentPixel.setX(0);
  currentPixel.setY(0);
  colour = image.pixelColor(currentPixel).rgb();
  emit mouseClicked();
}

const QImage &QLabelMouseTracker::getImage() const { return image; }

const QRgb &QLabelMouseTracker::getColour() const { return colour; }

void QLabelMouseTracker::mouseMoveEvent(QMouseEvent *event) {
  // on mouse move: update current pixel
  currentPixel.setX((pixmap.width() * event->pos().x()) / this->size().width());
  currentPixel.setY((pixmap.height() * event->pos().y()) /
                    this->size().height());
  // qDebug("QLabelMouseTracker :: currentPixel %d,%d", currentPixel.x(),
  // currentPixel.y());
  // todo: highlight/colour currently selected colour region on mouseover?

  if (event->buttons() != Qt::NoButton) {
    // on mouse click: update current colour and compartment ID
    if (!pixmap.isNull()) {
      colour = image.pixelColor(currentPixel).rgb();
      qDebug("QLabelMouseTracker :: mouseclick at colour %u", colour);
      emit mouseClicked();
    }
  }
}

void QLabelMouseTracker::resizeEvent(QResizeEvent *event) {
  if (!pixmap.isNull()) {
    this->setPixmap(
        pixmap.scaledToWidth(event->size().width(), Qt::FastTransformation));
  }
}
