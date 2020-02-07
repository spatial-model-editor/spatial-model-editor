#pragma once

#include <QDialog>
#include <memory>

namespace Ui {
class DialogImageSlice;
}

class DialogImageSlice : public QDialog {
  Q_OBJECT

 public:
  explicit DialogImageSlice(const QVector<QImage>& images,
                            const QVector<double>& timepoints,
                            QWidget* parent = nullptr);
  ~DialogImageSlice();
  QImage getSlicedImage() const;

 private:
  std::unique_ptr<Ui::DialogImageSlice> ui;
  const QVector<QImage>& imgs;
  const QVector<double>& time;
  QImage slice;
  void hslideTime_valueChanged(int value);
  void sliceAtX(int x);
  void sliceAtY(int y);
  void lblImage_mouseOver(const QPoint& point);
};
