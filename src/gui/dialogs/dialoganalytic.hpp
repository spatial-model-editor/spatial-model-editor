#pragma once

#include <QDialog>
#include <map>
#include <memory>
#include <string>
#include <utility>

#include "model.hpp"
#include "utils.hpp"

namespace Ui {
class DialogAnalytic;
}

namespace units {
class ModelUnits;
}

namespace Model {
struct SpatialCoordinates;
}

class DialogAnalytic : public QDialog {
  Q_OBJECT

public:
  explicit DialogAnalytic(const QString &analyticExpression,
                          const model::SpeciesGeometry &speciesGeometry,
                          model::ModelMath &modelMath,
                          const model::SpatialCoordinates &spatialCoordinates,
                          QWidget *parent = nullptr);
  ~DialogAnalytic();
  const std::string &getExpression() const;
  bool isExpressionValid() const;

private:
  std::unique_ptr<Ui::DialogAnalytic> ui;
  std::map<const std::string, std::pair<double, bool>> sbmlVars;
  std::string xId{};
  std::string yId{};
  // user supplied data
  const std::vector<QPoint> &points;
  double width;
  QPointF origin;
  QString lengthUnit;
  QString concentrationUnit;

  QImage img;
  utils::QPointIndexer qpi;
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
