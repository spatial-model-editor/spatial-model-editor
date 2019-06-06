#include "qlabelmousetracker.h"
#include <QDebug>

QLabelMousetracker::QLabelMousetracker(QWidget *parent) : QLabel(parent) {}

void QLabelMousetracker::setImage(const QImage &img) {
  image = img;
  pixmap = QPixmap::fromImage(img);
  this->setPixmap(pixmap.scaledToWidth(this->width(), Qt::FastTransformation));

  // set first mouse click to top left corner of image
  currentPixel.setX(0);
  currentPixel.setY(0);
  colour = image.pixelColor(currentPixel).rgb();
  emit mouseClicked();
}

const QImage &QLabelMousetracker::getImage() const { return image; }

const QRgb &QLabelMousetracker::getColour() const { return colour; }

void QLabelMousetracker::setMaskColour(QRgb colour) {
  maskColour = colour;
  maskImage = image.createMaskFromColor(maskColour, Qt::MaskOutColor);
}

const QImage &QLabelMousetracker::getMask() const { return maskImage; }

void QLabelMousetracker::mouseMoveEvent(QMouseEvent *event) {
  // on mouse move: update current pixel
  currentPixel.setX((pixmap.width() * event->pos().x()) / this->size().width());
  currentPixel.setY((pixmap.height() * event->pos().y()) /
                    this->size().height());

  // todo: highlight/colour currently selected colour region on mouseover?

  if (event->buttons() != Qt::NoButton) {
    // on mouse click: update current colour and compartment ID
    if (!pixmap.isNull()) {
      colour = image.pixelColor(currentPixel).rgb();
      emit mouseClicked();
    }
  }
}

void QLabelMousetracker::resizeEvent(QResizeEvent *event) {
  if (!pixmap.isNull()) {
    this->setPixmap(
        pixmap.scaledToWidth(event->size().width(), Qt::FastTransformation));
  }
}
