// QTabSimulate

#pragma once

#include <QWidget>
#include <memory>

#include "simulate.hpp"

namespace Ui {
class QTabSimulate;
}
class QCustomPlot;
class QCPAbstractPlottable;
class QCPItemStraightLine;
class QCPTextElement;
class QLabelMouseTracker;
namespace sbml {
class SbmlDocWrapper;
}

class QTabSimulate : public QWidget {
  Q_OBJECT

 public:
  explicit QTabSimulate(sbml::SbmlDocWrapper &doc,
                        QLabelMouseTracker *mouseTracker,
                        QWidget *parent = nullptr);
  ~QTabSimulate();
  void loadModelData();
  void stopSimulation();
  void useDune(bool enable);
  void reset();

 private:
  std::unique_ptr<Ui::QTabSimulate> ui;
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
  bool normaliseImageIntensityOverWholeSimulation = false;
  std::vector<bool> speciesVisible;
  bool plotShowMinMax = true;
  bool isSimulationRunning = false;

  void btnSimulate_clicked();
  void btnSliceImage_clicked();
  void updateSpeciesToDraw();
  void btnDisplayOptions_clicked();
  void graphClicked(const QMouseEvent *event);
  void hslideTime_valueChanged(int value);
};
