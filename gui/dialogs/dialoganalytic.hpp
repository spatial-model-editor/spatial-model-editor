#pragma once

#include "sme/utils.hpp"
#include <QDialog>
#include <map>
#include <memory>
#include <string>
#include <utility>

namespace Ui {
class DialogAnalytic;
}

namespace sme::model {
struct SpeciesGeometry;
class ModelParameters;
class ModelFunctions;
} // namespace sme::model

class DialogAnalytic : public QDialog {
  Q_OBJECT

public:
  explicit DialogAnalytic(const QString &analyticExpression,
                          const sme::model::SpeciesGeometry &speciesGeometry,
                          const sme::model::ModelParameters &modelParameters,
                          const sme::model::ModelFunctions &modelFunctions,
                          bool invertYAxis, QWidget *parent = nullptr);
  ~DialogAnalytic() override;
  const std::string &getExpression() const;
  const QImage &getImage() const;
  bool isExpressionValid() const;

private:
  std::unique_ptr<Ui::DialogAnalytic> ui;
  // user supplied data
  const std::vector<QPoint> &points;
  double width;
  QPointF origin;
  QString lengthUnit;
  QString concentrationUnit;

  QImage img;
  sme::common::QPointIndexer qpi;
  std::vector<double> concentration;
  std::string displayExpression;
  std::string variableExpression;
  bool expressionIsValid = false;

  QPointF physicalPoint(const QPoint &pixelPoint) const;
  void txtExpression_mathChanged(const QString &math, bool valid,
                                 const QString &errorMessage);
  void lblImage_mouseOver(QPoint point);
  void btnExportImage_clicked();
};
