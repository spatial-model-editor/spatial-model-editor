#pragma once
#include "sme/simulate_options.hpp"
#include <qcustomplot.h>

class QWidget;

struct PlotWrapperObservable {
  QString name;
  QString expression;
  bool visible;
};

class PlotWrapper {
private:
  QCPItemStraightLine *verticalLine;
  QCPTextElement *title;
  std::vector<std::string> species;
  std::vector<PlotWrapperObservable> observables;
  std::vector<std::vector<sme::simulate::AvgMinMax>> concs;
  QVector<double> times;
  std::vector<std::string> featureNames;
  int featureGraphOffset{0};

public:
  QCustomPlot *plot;
  explicit PlotWrapper(const QString &plotTitle, QWidget *parent = nullptr);
  void addAvMinMaxLine(const QString &name, QColor col);
  void addAvMinMaxPoint(int lineIndex, double time,
                        const sme::simulate::AvgMinMax &concentration);
  void addFeatureLine(const QString &name, QColor col,
                      QCPScatterStyle::ScatterShape scatterShape =
                          QCPScatterStyle::ScatterShape::ssTriangle);
  void addFeaturePoint(int featureLineIndex, double time, double value);
  void addObservableLine(const PlotWrapperObservable &plotWrapperObservable,
                         const QColor &col);
  void clearObservableLines();
  void setVerticalLine(double x);
  void update(const std::vector<bool> &speciesVisible, bool showMinMax,
              const std::vector<bool> &featureVisible = {});
  void clear();
  double xValue(const QMouseEvent *event) const;
  const QVector<double> &getTimepoints() const;
  QString getDataAsCSV() const;
};
