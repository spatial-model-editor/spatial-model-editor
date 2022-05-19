#pragma once

#include "sme/model.hpp"
#include "sme/utils.hpp"
#include <QDialog>
#include <memory>

namespace Ui {
class DialogConcentrationImage;
}

class DialogConcentrationImage : public QDialog {
  Q_OBJECT

public:
  explicit DialogConcentrationImage(
      const std::vector<double> &concentrationArray,
      const sme::model::SpeciesGeometry &speciesGeometry,
      bool invertYAxis = false, QWidget *parent = nullptr);
  ~DialogConcentrationImage() override;
  [[nodiscard]] std::vector<double> getConcentrationArray() const;

private:
  std::unique_ptr<Ui::DialogConcentrationImage> ui;

  // user supplied data
  const std::vector<QPoint> &points;
  double width;
  QPointF origin;
  QString lengthUnit;
  QString concentrationUnit;

  QImage colourMaxConc;
  QImage colourMinConc;
  QImage img;
  sme::common::QPointIndexer qpi;
  std::vector<double> concentration;

  QPointF physicalPoint(const QPoint &pixelPoint) const;
  std::size_t pointToConcentrationArrayIndex(const QPoint &point) const;
  void importConcentrationArray(const std::vector<double> &concentrationArray);
  void importConcentrationImage(const QImage &concentrationImage);
  void updateImageFromConcentration();
  void rescaleConcentration(double newMin, double newMax);
  void gaussianFilter(const QPoint &direction, double sigma);
  void smoothConcentration();
  void lblImage_mouseOver(QPoint point);
  void chkGrid_stateChanged(int state);
  void chkScale_stateChanged(int state);
  void btnImportImage_clicked();
  void btnExportImage_clicked();
  void cmbExampleImages_currentTextChanged(const QString &text);
};
