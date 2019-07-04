#include "qlabelmousetracker.hpp"

#include <QDebug>

#include "logger.hpp"

QLabelMouseTracker::QLabelMouseTracker(QWidget *parent) : QLabel(parent) {
  setMouseTracking(true);
  pixmap.fill();
}

void QLabelMouseTracker::setImage(const QImage &img) {
  image = img;
  pixmap = QPixmap::fromImage(image);
  if (!img.isNull()) {
    this->setPixmap(
        pixmap.scaledToWidth(this->width(), Qt::FastTransformation));
    // set first mouse click to top left corner of image
    currentPixel.setX(0);
    currentPixel.setY(0);
    colour = image.pixelColor(currentPixel).rgb();
    emit mouseClicked(colour);
  }
}

const QImage &QLabelMouseTracker::getImage() const { return image; }

const QRgb &QLabelMouseTracker::getColour() const { return colour; }

void QLabelMouseTracker::mouseMoveEvent(QMouseEvent *ev) {
  setCurrentPixel(ev);
  // possible todo: highlight colour region on mouseover?
}

void QLabelMouseTracker::mousePressEvent(QMouseEvent *ev) {
  if (ev->buttons() != Qt::NoButton) {
    setCurrentPixel(ev);
    // update current colour and emit mouseClicked signal
    if (!pixmap.isNull()) {
      colour = image.pixelColor(currentPixel).rgb();
      spdlog::debug(
          "QLabelMouseTracker::::mousePressEvent pixel ({},{}) -> colour {:x}",
          currentPixel.x(), currentPixel.y(), colour);
      emit mouseClicked(colour);
    }
  }
}

void QLabelMouseTracker::resizeEvent(QResizeEvent *event) {
  if (!pixmap.isNull()) {
    qDebug() << "QLabelMouseTracker::resizeEvent:" << event->size();
    this->setPixmap(
        pixmap.scaledToWidth(event->size().width(), Qt::FastTransformation));
  }
}

void QLabelMouseTracker::setCurrentPixel(QMouseEvent *ev) {
  currentPixel.setX((pixmap.width() * ev->pos().x()) / this->size().width());
  currentPixel.setY((pixmap.height() * ev->pos().y()) / this->size().height());
  qDebug() << "QLabelMouseTracker::setCurrentPixel : mouse at" << ev->pos()
           << "-> pixel" << currentPixel;
}
