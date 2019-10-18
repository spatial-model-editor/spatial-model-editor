#pragma once

#include <QDialog>
#include <memory>

#include "units.hpp"

namespace Ui {
class DialogImageSize;
}

class DialogImageSize : public QDialog {
  Q_OBJECT

 public:
  explicit DialogImageSize(const QImage& image, double pixelWidth,
                           const units::UnitVector& lengthUnit,
                           QWidget* parent = nullptr);
  double getPixelWidth() const;

 private:
  std::shared_ptr<Ui::DialogImageSize> ui;
  const QImage& img;
  double pixelLocalUnits;
  double pixelModelUnits;
  const units::UnitVector& length;
  QString modelUnitSymbol;

  void updateAll();
  void txtImageWidth_editingFinished();
  void txtImageHeight_editingFinished();
};
