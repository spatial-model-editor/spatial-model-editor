// QLabelMouseTracker
//  - a modified QLabel
//  - displays (and rescales without interpolation) an image,
//  - tracks the mouse location in terms of the pixels of the original image
//  - provides the colour of the last pixel that was clicked on
//  - provides a mask of the image with this colour
//  - emits a signal when the user clicks the mouse

#ifndef QLABEL_MOUSE_TRACKER_H
#define QLABEL_MOUSE_TRACKER_H
#include <QLabel>
#include <QMouseEvent>

class QLabelMouseTracker : public QLabel {
  Q_OBJECT
 public:
  QLabelMouseTracker(QWidget *parent);
  void setImage(const QImage &img);
  const QImage &getImage() const;
  const QRgb &getColour() const;
  void setMaskColour(QRgb colour);
  const QImage &getMask() const;

 signals:
  void mouseClicked();

 protected:
  void mouseMoveEvent(QMouseEvent *event) override;
  void resizeEvent(QResizeEvent *event) override;

 private:
  // (x,y) location of current pixel
  QPoint currentPixel;
  // colour of pixel at last mouse click position
  QRgb colour;
  // QImage used for pixel location and colour
  QImage image;
  // colour used for image mask
  QRgb maskColour;
  // masked image
  QImage maskImage;
  // Pixmap used to display scaled version of image
  QPixmap pixmap = QPixmap(1, 1);
};

#endif  // QLABEL_MOUSE_TRACKER_H
