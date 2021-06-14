// QLabelSlice
//  - a modified QLabel
//  - displays (and rescales without interpolation) an image,
//  - tracks the mouse location in terms of the pixels of the original image
//  - provides the colour of the last pixel that was clicked on
//  - emits a signal when the user clicks the mouse, along with the colour of
//  the pixel that was clicked on

#pragma once

#include <QLabel>
#include <QMouseEvent>
#include <vector>

class QLabelSlice : public QLabel {
  Q_OBJECT
public:
  explicit QLabelSlice(QWidget *parent = nullptr);
  void setImage(const QImage &img, bool invertYAxis = false);
  const QImage &getImage() const;
  void setAspectRatioMode(Qt::AspectRatioMode aspectRatioMode);
  void setTransformationMode(Qt::TransformationMode transformationMode);
  void setSlice(const QPoint &start, const QPoint &end);
  const std::vector<QPoint> &getSlicePixels() const;
  void setHorizontalSlice(int y);
  void setVerticalSlice(int x);

signals:
  void sliceDrawn(QPoint start, QPoint end);
  void mouseOver(QPoint point);
  void mouseDown(QPoint point);
  void mouseWheelEvent(int delta);

protected:
  void mousePressEvent(QMouseEvent *ev) override;
  void mouseMoveEvent(QMouseEvent *ev) override;
  void mouseReleaseEvent(QMouseEvent *ev) override;
  void mouseDoubleClickEvent(QMouseEvent *ev) override;
  void wheelEvent(QWheelEvent *ev) override;
  void resizeEvent(QResizeEvent *ev) override;

private:
  std::vector<QPoint> slicePixels;
  QImage imgOriginal;
  QImage imgSliced;
  QPixmap pixmap;
  QPoint startPixel;
  QPoint currentPixel;
  Qt::AspectRatioMode aspectRatioMode = Qt::KeepAspectRatio;
  Qt::TransformationMode transformationMode = Qt::FastTransformation;
  bool mouseIsDown{false};
  bool flipYAxis{false};
  bool setPixel(const QMouseEvent *ev, QPoint &pixel);
  void fadeOriginalImage();
  void resizeImage(const QSize &size);
};
