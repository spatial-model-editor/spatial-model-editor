// QLabelMouseTracker
//  - a modified QLabel
//  - displays (and rescales without interpolation) an image,
//  - tracks the mouse location in terms of the pixels of the original image
//  - provides the colour of the last pixel that was clicked on
//  - emits a signal when the user clicks the mouse, along with the colour of
//  the pixel that was clicked on

#pragma once

#include <QLabel>
#include <QMouseEvent>

class QLabelMouseTracker : public QLabel {
  Q_OBJECT
 public:
  explicit QLabelMouseTracker(QWidget *parent = nullptr);
  // QImage used for pixel location and colour
  void setImage(const QImage &img);
  const QImage &getImage() const;
  // QImage mask used to translate pixel location to index
  void setMaskImage(const QImage &img);
  const QImage &getMaskImage() const;
  void setImages(const std::pair<QImage, QImage> &imgPair);
  // colour of pixel at last mouse click position
  const QRgb &getColour() const;
  // value of mask index at last mouse click position
  int getMaskIndex() const;
  void setAspectRatioMode(Qt::AspectRatioMode aspectRatioMode);
  void setTransformationMode(Qt::TransformationMode transformationMode);

 signals:
  void mouseClicked(QRgb col, QPoint point);
  void mouseOver(QPoint point);
  void mouseWheelEvent(QWheelEvent *ev);

 protected:
  void mouseMoveEvent(QMouseEvent *ev) override;
  void mousePressEvent(QMouseEvent *ev) override;
  void resizeEvent(QResizeEvent *ev) override;
  void wheelEvent(QWheelEvent *ev) override;

 private:
  Qt::AspectRatioMode aspectRatioMode = Qt::KeepAspectRatio;
  Qt::TransformationMode transformationMode = Qt::FastTransformation;
  // (x,y) location of current pixel
  bool setCurrentPixel(const QMouseEvent *ev);
  void resizeImage(const QSize &size);
  QImage image;
  // Pixmap used to display scaled version of image
  QPixmap pixmap;
  QImage maskImage;
  QPoint currentPixel;
  QRgb colour;
  int maskIndex;
};
