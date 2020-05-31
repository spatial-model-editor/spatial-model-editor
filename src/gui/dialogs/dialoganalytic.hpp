#pragma once

#include <QDialog>
#include <memory>

#include "model.hpp"
#include "utils.hpp"

namespace Ui {
class DialogAnalytic;
}

namespace units {
class ModelUnits;
}

class DialogAnalytic : public QDialog {
  Q_OBJECT

 public:
  explicit DialogAnalytic(const QString& analyticExpression,
                          const model::SpeciesGeometry& speciesGeometry,
                          const std::vector<model::IdNameValue>& constants = {},
                          QWidget* parent = nullptr);
  ~DialogAnalytic();
  const std::string& getExpression() const;
  bool isExpressionValid() const;

 private:
  std::unique_ptr<Ui::DialogAnalytic> ui;

  // user supplied data
  const std::vector<QPoint>& points;
  double width;
  QPointF origin;
  QString lengthUnit;
  QString concentrationUnit;
  std::vector<double> vars;

  QImage img;
  utils::QPointIndexer qpi;
  std::vector<double> concentration;
  std::string displayExpression;
  std::string variableExpression;
  bool expressionIsValid = false;

  QPointF physicalPoint(const QPoint& pixelPoint) const;
  void txtExpression_mathChanged(const QString& math, bool valid,
                                 const QString& errorMessage);
  void lblImage_mouseOver(QPoint point);
  void btnExportImage_clicked();
};
