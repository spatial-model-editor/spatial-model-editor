#pragma once

#include "model_units.hpp"
#include <QDialog>
#include <memory>

namespace Ui {
class DialogGeometryImage;
}

class DialogGeometryImage : public QDialog {
  Q_OBJECT

public:
  explicit DialogGeometryImage(const QImage &image, double pixelWidth,
                               double pixelDepth,
                               const sme::model::ModelUnits &modelUnits,
                               QWidget *parent = nullptr);
  ~DialogGeometryImage() override;
  [[nodiscard]] double getPixelWidth() const;
  [[nodiscard]] double getPixelDepth() const;
  [[nodiscard]] bool imageAltered() const;
  [[nodiscard]] const QImage &getAlteredImage() const;

private:
  std::unique_ptr<Ui::DialogGeometryImage> ui;
  const QImage &originalImage;
  QImage coloredImage;
  QImage rescaledImage;
  bool alteredSize{false};
  bool alteredColours{false};
  bool selectingColours{false};
  QVector<QRgb> colorTable;
  double pixelLocalUnits;
  double pixelModelUnits;
  double depthLocalUnits;
  double depthModelUnits;
  const sme::model::ModelUnits &units;
  QString modelUnitSymbol;

  void updatePixelSize();
  void updateColours();
  void enableWidgets(bool enable);
  void lblImage_mouseClicked(QRgb col, QPoint point);
  void txtImageWidth_editingFinished();
  void txtImageHeight_editingFinished();
  void txtImageDepth_editingFinished();
  void spinPixelsX_valueChanged(int value);
  void spinPixelsY_valueChanged(int value);
  void btnResetPixels_clicked();
  void btnSelectColours_clicked();
  void btnApplyColours_clicked();
  void btnResetColours_clicked();
};
