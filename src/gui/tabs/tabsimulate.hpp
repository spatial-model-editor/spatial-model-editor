// TabSimulate

#pragma once

#include <QWidget>
#include <memory>

#include "simulate.hpp"

namespace Ui {
class TabSimulate;
}
class QCustomPlot;
class QCPAbstractPlottable;
class QCPItemStraightLine;
class QCPTextElement;
class QLabelMouseTracker;
namespace sbml {
class SbmlDocWrapper;
}

class TabSimulate : public QWidget {
  Q_OBJECT

 public:
  explicit TabSimulate(sbml::SbmlDocWrapper &doc,
                       QLabelMouseTracker *mouseTracker,
                       QWidget *parent = nullptr);
  ~TabSimulate();
  void loadModelData();
  void stopSimulation();
  void useDune(bool enable);
  void reset();
  simulate::IntegratorOptions getIntegratorOptions() const;
  void setIntegratorOptions(const simulate::IntegratorOptions &options);

 private:
  std::unique_ptr<Ui::TabSimulate> ui;
  sbml::SbmlDocWrapper &sbmlDoc;
  QLabelMouseTracker *lblGeometry;
  QCustomPlot *pltPlot;
  QCPTextElement *pltTitle;
  QCPItemStraightLine *pltTimeLine;
  std::unique_ptr<simulate::Simulation> sim;
  simulate::SimulatorType simType = simulate::SimulatorType::DUNE;
  QVector<double> time;
  QVector<QImage> images;
  QStringList compartmentNames;
  std::vector<QStringList> speciesNames;
  std::vector<std::vector<std::size_t>> compartmentSpeciesToDraw;
  bool normaliseImageIntensityOverWholeSimulation = true;
  std::vector<bool> speciesVisible;
  bool plotShowMinMax = true;
  bool isSimulationRunning = false;
  simulate::IntegratorOptions integratorOptions;

  void btnSimulate_clicked();
  void btnSliceImage_clicked();
  void updateSpeciesToDraw();
  void updatePlotAndImages();
  void btnDisplayOptions_clicked();
  void graphClicked(const QMouseEvent *event);
  void hslideTime_valueChanged(int value);
};
