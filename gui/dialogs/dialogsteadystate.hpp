#pragma once

#include "sme/model.hpp"
#include "sme/simulate_steadystate.hpp"

#include <QDialog>
#include <QElapsedTimer>
#include <QStringList>
#include <QTimer>

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
  std::unique_ptr<Ui::DialogSteadystate> ui;
  sme::simulate::SteadyStateSimulation sim;
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

public:
  explicit DialogSteadystate(sme::model::Model &model,
                             QWidget *parent = nullptr);
  ~DialogSteadystate();
};
