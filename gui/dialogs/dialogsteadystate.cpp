#include "dialogsteadystate.hpp"
#include "dialogdisplayoptions.hpp"
#include "ui_dialogdisplayoptions.h"
#include "ui_dialogsteadystate.h"

#include "sme/utils.hpp"

#include <chrono>
#include <limits>
#include <memory>

#include <qcombobox.h>
#include <qcustomplot.h>
#include <qline.h>
#include <qlineedit.h>
#include <qmessagebox.h>
#include <qvalidator.h>

#include <spdlog/spdlog.h>

////////////////////////////////////////////////////////////////////////////////
// helper functions
void DialogSteadystate::initUi() {
  // set up ui elements
  ui->setupUi(this);
  ui->splitter->setSizes({1000, 1000});

  // add options and defaults to the comboboxes
  ui->cmbConvergence->addItems({"Absolute", "Relative"});
  ui->cmbConvergence->setCurrentIndex(1);

  ui->cmbPlotting->addItems({"2D", "3D"});
  ui->cmbPlotting->setCurrentIndex(0);

  m_plotRefreshTimer.setTimerType(Qt::TimerType::VeryCoarseTimer);

  // set validators for input to only allow certain types in the QLineEdits
  ui->timeoutInput->setValidator(
      new QDoubleValidator(0, std::numeric_limits<double>::max(), 2, this));
  ui->timeoutInput->setText(QString::number(m_sim.getTimeout() / 1000));

  ui->toleranceInput->setValidator(
      new QDoubleValidator(0, std::numeric_limits<double>::max(), 2, this));
  ui->toleranceInput->setText(QString::number(m_sim.getStopTolerance()));

  ui->tolStepInput->setValidator(
      new QIntValidator(0, std::numeric_limits<int>::max(), this));
  ui->tolStepInput->setText(QString::number(m_sim.getStepsToConvergence()));

  ui->convIntervalInput->setValidator(
      new QDoubleValidator(0, std::numeric_limits<double>::max(), 2, this));
  ui->convIntervalInput->setText(QString::number(m_sim.getDt()));

  ui->valuesPlot->setZSlider(ui->zaxis);
}

void DialogSteadystate::connectSlots() {
  // connect slots and signals
  connect(ui->cmbConvergence, &QComboBox::currentIndexChanged, this,
          &DialogSteadystate::convergenceCurrentIndexChanged);

  connect(ui->cmbPlotting, &QComboBox::currentIndexChanged, this,
          &DialogSteadystate::plottingCurrentIndexChanged);

  connect(ui->timeoutInput, &QLineEdit::textChanged, this,
          &DialogSteadystate::timeoutInputChanged);

  connect(ui->toleranceInput, &QLineEdit::textChanged, this,
          &DialogSteadystate::toleranceInputChanged);

  connect(ui->tolStepInput, &QLineEdit::textChanged, this,
          &DialogSteadystate::stepsWithinToleranceInputChanged);

  connect(ui->convIntervalInput, &QLineEdit::textChanged, this,
          &DialogSteadystate::convIntervalInputChanged);

  connect(ui->btnStartStop, &QPushButton::clicked, this,
          &DialogSteadystate::btnStartStopClicked);

  connect(ui->btnReset, &QPushButton::clicked, this,
          &DialogSteadystate::btnResetClicked);

  connect(ui->btnDisplayOptions, &QPushButton::clicked, this,
          &DialogSteadystate::displayOptionsClicked);

  connect(ui->buttonBox, &QDialogButtonBox::accepted, this,
          &DialogSteadystate::btnOkClicked);

  connect(ui->buttonBox, &QDialogButtonBox::rejected, this,
          &DialogSteadystate::btnCancelClicked);

  connect(ui->zaxis, &QSlider::valueChanged, this,
          &DialogSteadystate::zaxisValueChanged);

  // set up gui update slot: update Gui every second
  m_plotRefreshTimer.setTimerType(Qt::TimerType::VeryCoarseTimer);
  m_plotRefreshTimer.setInterval(1000);
  connect(&m_plotRefreshTimer, &QTimer::timeout, this,
          &DialogSteadystate::plotUpdateTimerTimeout);
}

void DialogSteadystate::initErrorPlot() {
  QCPGraph *errorPlotParams = ui->errorPlot->addGraph();
  errorPlotParams->setPen(sme::common::indexedColors()[0]);
  errorPlotParams->setScatterStyle(
      QCPScatterStyle(QCPScatterStyle::ScatterShape::ssDisc, 5));
  ui->errorPlot->graph(0)->setName("Error");

  QCPGraph *tolerancePlotParams = ui->errorPlot->addGraph();
  tolerancePlotParams->setPen(sme::common::indexedColors()[1]);
  tolerancePlotParams->setScatterStyle(
      QCPScatterStyle(QCPScatterStyle::ScatterShape::ssCross, 5));
  ui->errorPlot->graph(1)->setName("Tolerance");

  ui->errorPlot->setInteraction(QCP::iRangeDrag, true);
  ui->errorPlot->setInteraction(QCP::iRangeZoom, true);
  ui->errorPlot->legend->setVisible(true);
  ui->errorPlot->xAxis->setLabel("Steps");
  ui->errorPlot->yAxis->setLabel("Error");
  ui->errorPlot->yAxis->setScaleType(QCPAxis::stLogarithmic);
  ui->errorPlot->rescaleAxes(true);
  ui->errorPlot->replot();
}

void DialogSteadystate::initConcPlot() {
  ui->valuesPlot->setVisible(true);
  ui->valuesPlot->invertYAxis(m_model.getDisplayOptions().invertYAxis);
  ui->valuesPlot->displayScale(m_model.getDisplayOptions().showGeometryScale);
  ui->valuesPlot->displayGrid(m_model.getDisplayOptions().showGeometryGrid);
  ui->valuesPlot->setPhysicalUnits(m_model.getUnits().getLength().name);

  auto volume = m_model.getGeometry().getImages().volume();
  // manipulate UI to reduce clutter when solving 2D problem
  if (volume.depth() == 1) {
    SPDLOG_DEBUG("2D problem detected, adjusting UI");
    ui->cmbPlotting->setEnabled(false);
    ui->zaxis->hide();
    ui->zlabel->hide();
  } else {
    ui->zaxis->setMaximum(int(volume.depth() - 1));
    ui->zaxis->setMinimum(0);
    ui->zaxis->setValue(0);
  }
  m_vizmode = VizMode::_2D;
  ui->valuesPlot3D->hide();
  updatePlot();
}

void DialogSteadystate::resetPlots() {
  ui->errorPlot->clearGraphs();
  initErrorPlot();
  initConcPlot();
}

void DialogSteadystate::reset() {
  if (m_isRunning) {
    SPDLOG_DEBUG(" - cannot reset while simulation is running");
    QMessageBox::warning(this, "Reset",
                         "Cannot reset while simulation is running.");
    return;
  }

  // if the simulation future is valid, it has to be done before we can reset.
  // otherwise it's not running to begin with and can be reset
  if ((m_simulationFuture.valid() &&
       m_simulationFuture.wait_for(std::chrono::seconds(0)) ==
           std::future_status::ready) ||
      m_simulationFuture.valid() == false) {

    m_sim.reset();
    SPDLOG_DEBUG(" - simulation is complete, resetting. sim values: stopTol: "
                 "{}, stepsTilConvergence: {}, "
                 "stepsBelowTol: {}, timeout: {}",
                 m_sim.getStopTolerance(), m_sim.getStepsToConvergence(),
                 m_sim.getStepsBelowTolerance(), m_sim.getTimeout());
    ui->btnStartStop->setText("Start");
    m_isRunning = false;
    resetPlots();
  } else {
    SPDLOG_ERROR("  - unexpectedly cannot reset the simulation");
  }
}

void DialogSteadystate::runSim() {
  m_plotRefreshTimer.start();

  /// start simulation in a different thread
  m_simulationFuture = std::async(
      std::launch::async, &sme::simulate::SteadyStateSimulation::run, &m_sim);
}

void DialogSteadystate::updatePlot() {
  // update error plot
  auto error = m_sim.getLatestError();
  auto step = m_sim.getLatestStep();

  // numeric_limits used as an indicator for 'no data' here
  if (error < std::numeric_limits<double>::max()) {

    if (ui->errorPlot->graph(0)->dataCount() > 0) {
      // adjust x value to avoid overtyping
      auto lastX = ui->errorPlot->graph(0)->dataMainKey(
          ui->errorPlot->graph(0)->dataCount() - 1);

      if (step <= lastX) {
        step = lastX + step;
      }

      SPDLOG_DEBUG("  - update data: step: {}, error: {}", step, error);
    }

    SPDLOG_DEBUG(" - update data: step: {}, error: {}", step, error);
    ui->errorPlot->graph(0)->addData(step, error);
    ui->errorPlot->graph(1)->addData(step, m_sim.getStopTolerance());
    ui->errorPlot->rescaleAxes(true);
    ui->errorPlot->replot(QCustomPlot::RefreshPriority::rpQueuedReplot);
  } else {
    SPDLOG_DEBUG("  - no data to update");
  }

  // update image plot
  auto image = m_sim.getConcentrationImage(
      m_compartmentSpeciesToPlot,
      m_model.getDisplayOptions().normaliseOverAllSpecies);

  if (m_vizmode == VizMode::_2D) {
    SPDLOG_DEBUG("  - 2D mode image");
    ui->valuesPlot->setImage(image);
    ui->valuesPlot->setZIndex(ui->zaxis->value());
    ui->valuesPlot->repaint();
  } else {
    SPDLOG_DEBUG("  - 3D mode image");
    ui->valuesPlot3D->setImage(image);
    ui->valuesPlot3D->repaint();
  }
}

void DialogSteadystate::updateSpeciesToPlot() {
  m_compartmentSpeciesToPlot.clear();
  for (const auto &compSpecies : m_speciesNames) {
    std::vector<std::size_t> speciesToPlot;
    for (std::size_t i = 0; i < static_cast<std::size_t>(compSpecies.size());
         ++i) {
      if (m_displayoptions.showSpecies[i]) {
        speciesToPlot.push_back(i);
      }
    }
    m_compartmentSpeciesToPlot.emplace_back(std::move(speciesToPlot));
  }
}

void DialogSteadystate::finalise() {
  m_plotRefreshTimer.stop();
  ui->btnReset->setEnabled(true);
}

const sme::simulate::SteadyStateSimulation &
DialogSteadystate::getSimulator() const {
  return m_sim;
}

bool DialogSteadystate::isRunning() const { return m_isRunning; }

////////////////////////////////////////////////////////////////////////////////
// lifecycle
DialogSteadystate::DialogSteadystate(sme::model::Model &model, QWidget *parent)
    : QDialog(parent), m_model(model),
      ui(std::make_unique<Ui::DialogSteadystate>()),
      m_sim(sme::simulate::SteadyStateSimulation(
          model, 1e-6, 10, sme::simulate::SteadyStateConvergenceMode::relative,
          3600000, 1)) {

  SPDLOG_DEBUG("construct dialog steadystate with solver: {}",
               static_cast<int>(m_sim.getSimulatorType()));

  // set up bookkeeping data for display options
  m_compartmentSpeciesToPlot = m_sim.getCompartmentSpeciesIdxs();
  m_compartmentNames = m_model.getCompartments().getIds();
  m_speciesNames.resize(m_compartmentNames.size());
  std::size_t nSpecies = 0;
  for (std::size_t i = 0; i < m_compartmentNames.size(); ++i) {
    m_speciesNames[i] =
        sme::common::toQString(m_sim.getCompartmentSpeciesIds()[i]);
    nSpecies += m_speciesNames[i].size();
  }
  m_displayoptions = m_model.getDisplayOptions(); // get default display options
  if (m_displayoptions.showSpecies.size() != nSpecies) {
    // if there is a mismatch here just set all species to be visible
    m_displayoptions.showSpecies.resize(nSpecies, true);
  }

  // initialize ui elements and plotting panes
  initUi();
  initErrorPlot();
  initConcPlot();

  // connect slots and signals to make the ui functional
  connectSlots();
}

DialogSteadystate::~DialogSteadystate() = default;

////////////////////////////////////////////////////////////////////////////////
// slots
void DialogSteadystate::displayOptionsClicked() {
  DialogDisplayOptions dialog(m_compartmentNames, m_speciesNames,
                              m_displayoptions,
                              std::vector<PlotWrapperObservable>{}, this);

  // we don´t care about timepoints here
  dialog.ui->cmbNormaliseOverAllTimepoints->hide();

  // we don´t care about adding observables here either for now.
  dialog.ui->btnAddObservable->hide();
  dialog.ui->btnEditObservable->hide();
  dialog.ui->btnRemoveObservable->hide();
  dialog.ui->listObservables->hide();
  dialog.ui->chkShowMinMaxRanges->hide();

  if (dialog.exec() == QDialog::Accepted) {
    m_displayoptions.showMinMax = dialog.getShowMinMax();
    SPDLOG_DEBUG("  currently selected species: {}", dialog.getShowSpecies(),
                 m_speciesNames.size());
    m_displayoptions.showSpecies = dialog.getShowSpecies();
    m_displayoptions.normaliseOverAllTimepoints = false;
    m_displayoptions.normaliseOverAllSpecies =
        dialog.getNormaliseOverAllSpecies();
    m_model.setDisplayOptions(m_displayoptions);
    updateSpeciesToPlot();
    updatePlot();
  }
}

void DialogSteadystate::plotUpdateTimerTimeout() {

  updatePlot();

  // check if simulation has converged or stopped early
  if (m_simulationFuture.valid() &&
      m_simulationFuture.wait_for(std::chrono::seconds(0)) ==
          std::future_status::ready) {

    if (m_sim.hasConverged()) {
      SPDLOG_DEBUG("Simulation has converged: {}",
                   m_sim.getStepsBelowTolerance());
      m_isRunning = false;
      ui->btnStartStop->setText("Start");
      QMessageBox::information(this, "Simulation converged",
                               "The simulation has converged.");
      finalise();
    } else if (m_sim.getSolverStopRequested()) {
      m_isRunning = false;
      SPDLOG_DEBUG("Simulation has stopped early");
      if (m_sim.getSolverErrormessage() == "Simulation timed out") {
        QMessageBox::critical(this, "Simulation timed out",
                              m_sim.getSolverErrormessage().c_str());
      } else {
        QMessageBox::critical(this, "Simulation stopped early",
                              m_sim.getSolverErrormessage().c_str());
      }
      finalise();
    } else {
      // do nothing
      SPDLOG_DEBUG("not yet done simulating, nothing to do");
    }
  }
}

void DialogSteadystate::convergenceCurrentIndexChanged(int index) {

  if (index == 0) {
    m_sim.setConvergenceMode(
        sme::simulate::SteadyStateConvergenceMode::absolute);
  } else {
    m_sim.setConvergenceMode(
        sme::simulate::SteadyStateConvergenceMode::relative);
  }
}

void DialogSteadystate::plottingCurrentIndexChanged(
    [[maybe_unused]] int index) {

  if (m_vizmode == VizMode::_2D) {
    SPDLOG_DEBUG("2D -> 3D");
    ui->valuesPlot->hide();
    ui->valuesPlot3D->show();
    ui->zaxis->hide();
    ui->zlabel->hide();
    m_vizmode = VizMode::_3D;
  } else {
    SPDLOG_DEBUG("3D -> 2D");
    ui->valuesPlot->show();
    ui->valuesPlot3D->hide();
    ui->zaxis->show();
    ui->zlabel->show();
    ui->zaxis->setMaximum(
        int(m_model.getGeometry().getImages().volume().depth() - 1));
    ui->zaxis->setMinimum(0);
    ui->zaxis->setValue(0);
    ui->valuesPlot->setZIndex(ui->zaxis->value());
    m_vizmode = VizMode::_2D;
  }
  SPDLOG_DEBUG(" - m_vizmode: {}", static_cast<int>(m_vizmode));
}

void DialogSteadystate::timeoutInputChanged() {
  // input is in seconds, but we store it in milliseconds
  m_sim.setTimeout(ui->timeoutInput->text().toDouble() * 1000);
  SPDLOG_DEBUG("timeout change to {}, newly set: {}",
               ui->timeoutInput->text().toDouble(), m_sim.getTimeout());
}

void DialogSteadystate::toleranceInputChanged() {
  m_sim.setStopTolerance(ui->toleranceInput->text().toDouble());
  SPDLOG_DEBUG("tolerance changed to {}, newly set: {}",
               ui->toleranceInput->text().toDouble(), m_sim.getStopTolerance());
}

void DialogSteadystate::stepsWithinToleranceInputChanged() {
  m_sim.setStepsToConvergence(ui->tolStepInput->text().toInt());
  SPDLOG_DEBUG("steps within tolerance changed to {}, newly set: {}",
               ui->tolStepInput->text().toInt(), m_sim.getStepsToConvergence());
}

void DialogSteadystate::convIntervalInputChanged() {
  m_sim.setDt(ui->convIntervalInput->text().toDouble());
  SPDLOG_DEBUG("convergence interval changed to {}, newly set: {}",
               ui->convIntervalInput->text().toDouble(), m_sim.getDt());
}

void DialogSteadystate::btnStartStopClicked() {
  SPDLOG_DEBUG("start/stop clicked {}", m_isRunning);
  if (!m_isRunning) {
    SPDLOG_DEBUG(" start/resume simulation");
    m_isRunning = true;
    // start timer to periodically update simulation results
    ui->btnStartStop->setText("Stop");
    ui->btnReset->setEnabled(false);
    runSim();
  } else {
    SPDLOG_DEBUG(" stop simulation");
    m_isRunning = false;
    m_sim.requestStop();

    if (m_simulationFuture.valid()) {
      SPDLOG_DEBUG(" waiting for simulation to finish");
      m_simulationFuture.wait();
    }
    finalise();

    if (m_sim.hasConverged() == false) {
      ui->btnStartStop->setText("Resume");
    } else {
      ui->btnStartStop->setText("Start");
    }
    QMessageBox::information(this, "Simulation stopped",
                             "The simulation has been stopped.");
  }
}

void DialogSteadystate::btnResetClicked() { reset(); }

void DialogSteadystate::btnOkClicked() {
  SPDLOG_DEBUG("ok clicked");
  if (m_isRunning) {
    SPDLOG_DEBUG(" - simulation is running, stop it");
    m_sim.requestStop();
    m_simulationFuture.wait();
    m_isRunning = false;
  }
  finalise();

  SPDLOG_DEBUG(" - simulation is done, finalise");
  accept();
}

void DialogSteadystate::btnCancelClicked() {
  SPDLOG_DEBUG("cancel clicked");
  if (m_isRunning) {
    SPDLOG_DEBUG(" - simulation is running, stop it");
    m_sim.requestStop();
    m_simulationFuture.wait();
    m_isRunning = false;
  }
  finalise();
  reject();
}

void DialogSteadystate::zaxisValueChanged(int value) {
  SPDLOG_DEBUG("zaxis value changed to {}", value);
  ui->valuesPlot->setZIndex(value);
  updatePlot();
}
