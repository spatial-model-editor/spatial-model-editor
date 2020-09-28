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
namespace model {
class Model;
}

class TabSimulate : public QWidget {
  Q_OBJECT

public:
  explicit TabSimulate(model::Model &doc, QLabelMouseTracker *mouseTracker,
                       QWidget *parent = nullptr);
  ~TabSimulate();
  void loadModelData();
  void stopSimulation();
  void useDune(bool enable);
  void reset();
  simulate::Options getOptions() const;
  void setOptions(const simulate::Options &options);
  void setMaxThreads(std::size_t maxThreads);
  std::size_t getMaxThreads() const;

private:
  std::unique_ptr<Ui::TabSimulate> ui;
  model::Model &sbmlDoc;
  QLabelMouseTracker *lblGeometry;
  QCustomPlot *pltPlot;
  QCPTextElement *pltTitle;
  QCPItemStraightLine *pltTimeLine;
  std::unique_ptr<simulate::Simulation> sim;
  simulate::SimulatorType simType{simulate::SimulatorType::DUNE};
  simulate::Options simOptions;
  QVector<double> time;
  QVector<QImage> images;
  QStringList compartmentNames;
  std::vector<QStringList> speciesNames;
  std::vector<std::vector<std::size_t>> compartmentSpeciesToDraw;
  bool normaliseImageIntensityOverAllTimepoints{true};
  bool normaliseImageIntensityOverAllSpecies{true};
  std::vector<bool> speciesVisible;
  bool plotShowMinMax{true};
  bool isSimulationRunning{false};

  void btnSimulate_clicked();
  void btnSliceImage_clicked();
  void btnSaveImage_clicked();
  void updateSpeciesToDraw();
  void updatePlotAndImages();
  void btnDisplayOptions_clicked();
  void graphClicked(const QMouseEvent *event);
  void hslideTime_valueChanged(int value);
};
