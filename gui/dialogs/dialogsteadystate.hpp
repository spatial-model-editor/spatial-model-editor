#pragma once

#include "sme/model.hpp"
#include "sme/simulate_steadystate.hpp"

#include <QDialog>
#include <QElapsedTimer>
#include <QStringList>
#include <QTimer>

#include <future>
#include <memory>

namespace Ui {
class DialogSteadystate;
}

namespace sme::simulate {
class BaseSim;
}

class DialogSteadystate : public QDialog {
  Q_OBJECT
private:
  // struct that indicates visualization mode: 2d plane with zslider or full 3D
  enum struct VizMode {
    _2D,
    _3D,
  };

  // simulation parameters
  sme::model::Model &m_model;
  std::unique_ptr<Ui::DialogSteadystate> ui;
  sme::simulate::SteadyStateSimulation sim;
  std::future<void> m_simulationFuture;
  QTimer m_plotRefreshTimer;

  // helper for plotting
  VizMode vizmode;
  bool isRunning;

  // helpers for simulation

  // helper functions
  void initPlots();
  void update();
  void finalize();
  void selectZ();
  void resetPlots();
  void reset();
  void runAndPlot();

  // slots
  void convergenceCurrentIndexChanged(int index);
  void plottingCurrentIndexChanged(int index);
  void stepsWithinToleranceInputChanged();
  void timeoutInputChanged();
  void toleranceInputChanged();
  void convIntervalInputChanged();
  void btnStartStopClicked();
  void btnResetClicked();
  void btnOkClicked();
  void btnCancelClicked();
  void btnDisplayOptionsClicked();

public:
  explicit DialogSteadystate(sme::model::Model &model,
                             QWidget *parent = nullptr);
  ~DialogSteadystate();
};
