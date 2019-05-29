#include "qlabelmousetracker.h"
#include <QDebug>
#include <iostream>
#include <map>

QLabelMousetracker::QLabelMousetracker(QWidget *parent) : QLabel(parent) {}

void QLabelMousetracker::mouseMoveEvent(QMouseEvent *event) {
  // on mouse move: update current pixel
  current_pixel.setX((pixmap.width() * event->pos().x()) /
                     this->size().width());
  current_pixel.setY((pixmap.height() * event->pos().y()) /
                     this->size().height());

  /*
  pixmap = pixmap_original;
  QPainter painter(&pixmap);
  painter.setCompositionMode(QPainter::CompositionMode_HardLight);
  painter.drawImage(0, 0, mask[colour]);
  this->setPixmap(pixmap);
  // qDebug("%d %d", current_pixel.x(), current_pixel.y());
  */

  if (event->buttons() != Qt::NoButton) {
    // on mouse click: update current colour and compartment ID
    if (!pixmap.isNull()) {
      colour = image.pixelColor(current_pixel).rgb();
      compartmentID = colour_to_comp[colour];
      emit mouseClicked();
    }
  }
}

void QLabelMousetracker::importGeometry(const QString &filename) {
  pixmap.load(filename);
  image = pixmap.toImage();
  // convert geometry image to 8-bit indexed format
  // each pixel points to an index in the colorTable
  // which contains an RGB value for each color in the image
  image = image.convertToFormat(QImage::Format_Indexed8);
  qDebug() << image.format();
  qDebug() << image.colorCount();
  //
  auto compartments = image.colorTable();
  for (QRgb &c : compartments) {
    // mask[c] = image.createMaskFromColor(c, Qt::MaskOutColor);
    // QColor green;
    // green.setRgb(55, 99, 255);
    // QVector<QRgb> table {green.rgba()};
    // mask[c].setColorTable(table);
    colour_to_comp[c] = "none";
    qDebug() << c;
  }
  pixmap = QPixmap::fromImage(image);
  // pixmap_original = pixmap;
  this->setPixmap(pixmap.scaledToWidth(this->width(), Qt::FastTransformation));

  // set first mouse click to top left corner of image
  current_pixel.setX(0);
  current_pixel.setY(0);
  colour = image.pixelColor(current_pixel).rgb();
  compartmentID = colour_to_comp[colour];
  emit mouseClicked();
}

void QLabelMousetracker::resizeEvent(QResizeEvent *event) {
  if (!pixmap.isNull()) {
    this->setPixmap(
        pixmap.scaledToWidth(event->size().width(), Qt::FastTransformation));
  }
}
