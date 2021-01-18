#pragma once

#include <QDialog>
#include <memory>
#include "model_units.hpp"

namespace Ui {
class DialogImageSize;
}

class DialogImageSize : public QDialog {
  Q_OBJECT

public:
  explicit DialogImageSize(const QImage &image, double pixelWidth,
                           const sme::model::ModelUnits &modelUnits,
                           QWidget *parent = nullptr);
  ~DialogImageSize();
  double getPixelWidth() const;

private:
  std::unique_ptr<Ui::DialogImageSize> ui;
  const QImage &img;
  double pixelLocalUnits;
  double pixelModelUnits;
  const sme::model::ModelUnits &units;
  QString modelUnitSymbol;

  void updateAll();
  void txtImageWidth_editingFinished();
  void txtImageHeight_editingFinished();
};
