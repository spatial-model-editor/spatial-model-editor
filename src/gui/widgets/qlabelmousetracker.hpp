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
  // position of mouse as fraction of displayed pixmap size
  QPointF getRelativePosition() const;
  void setAspectRatioMode(Qt::AspectRatioMode aspectRatioMode);
  void setTransformationMode(Qt::TransformationMode transformationMode);
  void setPhysicalSize(const QSizeF& size, const QString& units);
  void displayGrid(bool enable);
  void displayScale(bool enable);

signals:
  void mouseClicked(QRgb col, QPoint point);
  void mouseOver(QPoint point);
  void mouseWheelEvent(QWheelEvent *ev);

protected:
  void mousePressEvent(QMouseEvent *ev) override;
  void mouseMoveEvent(QMouseEvent *ev) override;
  void mouseReleaseEvent(QMouseEvent *ev) override;
  void mouseDoubleClickEvent(QMouseEvent *ev) override;
  void wheelEvent(QWheelEvent *ev) override;
  void resizeEvent(QResizeEvent *ev) override;

private:
  Qt::AspectRatioMode aspectRatioMode = Qt::KeepAspectRatio;
  Qt::TransformationMode transformationMode = Qt::FastTransformation;
  bool setCurrentPixel(const QPoint& pos);
  void resizeImage(const QSize &size);
  QImage image;
  // Pixmap used to display scaled version of image
  QPixmap pixmap;
  // size of actual image in pixmap (may be smaller than pixmap)
  QSize pixmapImageSize;
  QImage maskImage;
  QPoint currentPixel{};
  const int tickLength{10};
  // x & y offsets for ruler, if present
  QPoint offset{0,0};
  bool drawGrid{false};
  bool drawScale{false};
  QSizeF physicalSize{1.0, 1.0};
  QString lengthUnits{};
  QRgb colour{};
  int maskIndex{};
};
