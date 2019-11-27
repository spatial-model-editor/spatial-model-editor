#pragma once

#include <QDialog>
#include <memory>

#include "sbml.hpp"
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
                          const sbml::SpeciesGeometry& speciesGeometry,
                          QWidget* parent = nullptr);
  const std::string& getExpression() const;
  bool isExpressionValid() const;

 private:
  std::shared_ptr<Ui::DialogAnalytic> ui;

  // user supplied data
  const std::vector<QPoint>& points;
  double width;
  QPointF origin;
  QString lengthUnit;
  QString concentrationUnit;

  QImage img;
  utils::QPointIndexer qpi;
  std::vector<double> concentration;
  std::string expression;
  bool expressionIsValid = false;

  QPointF physicalPoint(const QPoint& pixelPoint) const;
  void txtExpression_mathChanged(const QString& math, bool valid,
                                 const QString& errorMessage);
  void lblImage_mouseOver(QPoint point);
};
