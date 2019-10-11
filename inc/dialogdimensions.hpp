#pragma once

#include <QDialog>
#include <memory>

namespace Ui {
class DialogDimensions;
}

class DialogDimensions : public QDialog {
  Q_OBJECT

 public:
  explicit DialogDimensions(const QSize& imageSize, double pixelWidth,
                            QWidget* parent = nullptr);
  double getPixelWidth() const;
  bool resizeCompartments() const;

 private:
  std::shared_ptr<Ui::DialogDimensions> ui;
  QSize imgSize;
  double pixel;

  void updateAll();
  void txtImageWidth_editingFinished();
  void txtImageHeight_editingFinished();
};
