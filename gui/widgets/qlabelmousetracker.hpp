// QLabelMouseTracker
//  - a modified QLabel
//  - displays (and rescales without interpolation) a slice of an image stack,
//  - tracks the mouse location in terms of the pixels of the original image
//  - provides the color of the last pixel that was clicked on
//  - emits a signal when the user clicks the mouse, along with the color of
//  the pixel that was clicked on

#pragma once

#include "sme/image_stack.hpp"
#include <QLabel>
#include <QMouseEvent>
#include <QSlider>

class QLabelMouseTracker : public QLabel {
  Q_OBJECT
public:
  explicit QLabelMouseTracker(QWidget *parent = nullptr);
  // QImage used for pixel location and color
  void setImage(const sme::common::ImageStack &img);
  [[nodiscard]] const sme::common::ImageStack &getImage() const;
  // set which z-slice to display
  void setZIndex(int value);
  // optional QSlider to control which z-slice to display
  void setZSlider(QSlider *slider);
  [[nodiscard]] QSlider *getZSlider() const;
  // optional image mask used to translate pixel location to index
  void setMaskImage(const sme::common::ImageStack &img);
  [[nodiscard]] const sme::common::ImageStack &getMaskImage() const;
  void setImages(const std::pair<sme::common::ImageStack,
                                 sme::common::ImageStack> &imgPair);
  // color of pixel at last mouse click position
  [[nodiscard]] QRgb getColor() const;
  // value of mask index at last mouse click position
  [[nodiscard]] int getMaskIndex() const;
  // position of mouse as fraction of displayed pixmap volume
  [[nodiscard]] QPointF getRelativePosition() const;
  void setAspectRatioMode(Qt::AspectRatioMode aspectRatioMode);
  void setTransformationMode(Qt::TransformationMode transformationMode);
  void setPhysicalUnits(const QString &units);
  void displayGrid(bool enable);
  void displayScale(bool enable);
  void invertYAxis(bool enable);

signals:
  void mouseClicked(QRgb col, sme::common::Voxel voxel);
  void mouseOver(const sme::common::Voxel &voxel);
  void mouseWheelEvent(QWheelEvent *ev);

protected:
  void mousePressEvent(QMouseEvent *ev) override;
  void mouseMoveEvent(QMouseEvent *ev) override;
  void mouseReleaseEvent(QMouseEvent *ev) override;
  void mouseDoubleClickEvent(QMouseEvent *ev) override;
  void wheelEvent(QWheelEvent *ev) override;
  void resizeEvent(QResizeEvent *ev) override;

private:
  QSlider *zSlider{nullptr};
  Qt::AspectRatioMode aspectRatioMode = Qt::IgnoreAspectRatio;
  Qt::TransformationMode transformationMode = Qt::FastTransformation;
  bool setCurrentPixel(const QPoint &pos);
  void resizeImage(const QSize &size);
  sme::common::ImageStack image;
  // Pixmap used to display scaled version of image
  QPixmap pixmap;
  // size of actual image in pixmap (may be smaller than pixmap)
  QSize pixmapImageSize;
  sme::common::ImageStack maskImage;
  sme::common::Voxel currentVoxel{0, 0, 0};
  const int tickLength{10};
  // x & y offsets for ruler, if present
  QPoint offset{0, 0};
  bool drawGrid{false};
  bool drawScale{false};
  bool flipYAxis{false};
  sme::common::VolumeF physicalSize{1.0, 1.0, 1.0};
  QString lengthUnits{};
  QRgb color{};
  int maskIndex{};
};
