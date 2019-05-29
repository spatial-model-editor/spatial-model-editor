#ifndef MOUSEOVER_QLABEL_H
#define MOUSEOVER_QLABEL_H
#include <QLabel>
#include <QMouseEvent>
#include <QPainter>

class QLabelMousetracker : public QLabel {
  Q_OBJECT

 public:
  QLabelMousetracker(QWidget *parent);
  void importGeometry(const QString &filename);

  QRgb colour;
  QString compartmentID;
  // map from colour to compartment ID
  std::map<QRgb, QString> colour_to_comp;

 signals:
  void mouseClicked();

 protected:
  void mouseMoveEvent(QMouseEvent *event) override;
  void resizeEvent(QResizeEvent *event) override;

 private:
  // (x,y) location of current pixel
  QPoint current_pixel;
  // pixmap used for display to the user
  QPixmap pixmap;
  // original pixmap
  // QPixmap pixmap_original;
  // image used for direct pixel manipulation
  QImage image;
  // masks for highlighting each colour
  // std::map<QRgb, QImage> mask;
};

#endif  // MOUSEOVER_QLABEL_H
