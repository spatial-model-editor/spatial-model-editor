#pragma once

#include "sme/model.hpp"
#include "sme/simulate.hpp"
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
  enum struct VizMode {
    _2D,
    _3D,
  };
  sme::model::Model &m_model;
  std::unique_ptr<sme::simulate::BaseSim> m_simulator;
  std::unique_ptr<Ui::DialogSteadystate> ui;
  QTimer m_plotRefreshTimer;
  VizMode vizmode;
  std::vector<double> steps;
  std::vector<double> errors;

  // helpers
  void init_plots();
  void make_simulator();
  void reset();

  // slots
  void solverCurrentIndexChanged(int index);
  void convergenceCurrentIndexChanged(int index);
  void plottingCurrentIndexChanged(int index);
  void stepsWithinToleranceInputChanged();
  void maxstepsInputChanged();
  void timeoutInputChanged();
  void toleranceInputChanged();
  void btnStartStopClicked();
  void updatePlots();
  void finalizePlots();
  void selectZ();

public:
  explicit DialogSteadystate(sme::model::Model &model,
                             QWidget *parent = nullptr);
  ~DialogSteadystate() override;
};
