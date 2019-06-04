#ifndef MOUSEOVER_QLABEL_H
#define MOUSEOVER_QLABEL_H
#include <QLabel>
#include <QMouseEvent>

// A modified QLabel which:
// - displays (and rescales) an image,
// - tracks the mouse location in terms of the pixels of the original image
// - provides the colour of the last pixel that was clicked on
// - provides a mask of the image with this colour
// - emits a signal when the user clicks the mouse
class QLabelMousetracker : public QLabel {
  Q_OBJECT
 public:
  QLabelMousetracker(QWidget *parent);
  void setImage(const QImage &img);
  const QImage &getImage() const;
  const QRgb &getColour() const;
  void setMaskColour(QRgb colour);
  const QImage &getMask() const;
  // get colour of pixel at last mouse click position

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

#endif  // MOUSEOVER_QLABEL_H
