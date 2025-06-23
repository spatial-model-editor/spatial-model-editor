#pragma once

#include "plotwrapper.hpp"
#include "sme/model.hpp"
#include "sme/model_settings.hpp"
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

  // simulation parameters:
  sme::model::Model &m_model;
  std::unique_ptr<Ui::DialogSteadystate> ui;
  sme::simulate::SteadyStateSimulation m_sim;
  std::future<void> m_simulationFuture;
  QTimer m_plotRefreshTimer;
  QStringList m_compartmentNames;
  std::vector<QStringList> m_speciesNames;
  sme::model::DisplayOptions m_displayoptions;
  std::vector<std::vector<std::size_t>> m_compartmentSpeciesToPlot;

  // helpers for plotting:
  VizMode m_vizmode = VizMode::_2D;
  bool m_isRunning = false;

  // helpers for simulation

  // helper functions
  void initUi();
  void connectSlots();
  void initErrorPlot();
  void initConcPlot();
  void updatePlot();
  void finalise();
  void resetPlots();
  void reset();
  void runSim();
  void updateSpeciesToPlot();

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
  void plotUpdateTimerTimeout();
  void displayOptionsClicked();
  void zaxisValueChanged(int value);

public:
  /**
   * @brief Construct a new Dialog Steadystate object
   *
   * @param model Model to run the simulation on
   * @param parent Parent widget. Default is nullptr
   */
  explicit DialogSteadystate(sme::model::Model &model,
                             QWidget *parent = nullptr);
  ~DialogSteadystate() override;

  /**
   * @brief Get the Simulator object
   *
   * @return const sme::simulate::SteadyStateSimulation&
   */
  [[nodiscard]] const sme::simulate::SteadyStateSimulation &
  getSimulator() const;

  /**
   * @brief Check if a simulation is still running
   *
   * @return true
   * @return false
   */
  [[nodiscard]] bool isRunning() const;

  [[nodiscard]] bool getStopRequested() const { return m_sim.hasConverged(); }
};
