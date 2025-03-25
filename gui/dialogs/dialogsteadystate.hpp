#pragma once

#include "sme/model.hpp"
#include "sme/simulate_steadystate.hpp"
#include <QDialog>
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
  sme::SteadyStateSimulation sim;
  QTimer m_plotRefreshTimer;

  // helper for plotting
  VizMode vizmode;

  // helpers for simulation

  // helper functions
  void init_plots();
  void make_simulator();
  void updatePlots();
  void finalizePlots();
  void reset();
  void selectZ();

  // slots
  void solverCurrentIndexChanged(int index);
  void convergenceCurrentIndexChanged(int index);
  void plottingCurrentIndexChanged(int index);
  void stepsWithinToleranceInputChanged();
  void timeoutInputChanged();
  void toleranceInputChanged();
  void btnStartStopClicked();

public:
  explicit DialogSteadystate(sme::model::Model &model,
                             QWidget *parent = nullptr);
  ~DialogSteadystate() override;
};
