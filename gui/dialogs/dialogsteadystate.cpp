#include "dialogsteadystate.hpp"
#include "dialogdisplayoptions.hpp"
#include "ui_dialogdisplayoptions.h"
#include "ui_dialogsteadystate.h"

#include "sme/utils.hpp"

#include <chrono>
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
  // Force visibility changes to be processed
  QCoreApplication::processEvents();
  QTimer::singleShot(0, this, &DialogSteadystate::update);
}

void DialogSteadystate::resetPlots() {
  ui->errorPlot->clearGraphs();
  initErrorPlot();
  initConcPlot();
}

void DialogSteadystate::reset() {
  SPDLOG_CRITICAL("reset");

  if (m_isRunning) {
    SPDLOG_CRITICAL("  cannot reset while simulation is running");
    QMessageBox::warning(this, "Reset",
                         "Cannot reset while simulation is running.");
    return;
  }

  // if the simulation future is valid, it has to be done before we can reset.
  // otherwise it's not running to begin with and can be reset
  if ((m_simulationFuture.valid() and
       m_simulationFuture.wait_for(std::chrono::seconds(0)) ==
           std::future_status::ready) or
      not m_simulationFuture.valid()) {
    SPDLOG_CRITICAL("  simulation is complete, resetting");

    m_sim.reset();
    SPDLOG_CRITICAL("sim values: stopTol: {}, stepsTilConvergence: {}, "
                    "stepsBelowTol: {}, timeout: {}",
                    m_sim.getStopTolerance(), m_sim.getStepsToConvergence(),
                    m_sim.getStepsBelowTolerance(), m_sim.getTimeout());
    ui->btnStartStop->setText("Start");
    m_isRunning = false;
    resetPlots();
  } else {
    SPDLOG_CRITICAL("  - unexpectedly cannot reset the simulation");
  }
}

void DialogSteadystate::runAndPlot() {
  SPDLOG_CRITICAL(" run and plot");
  m_plotRefreshTimer.start();

  /// start simulation in a different thread
  m_simulationFuture = std::async(
      std::launch::async, &sme::simulate::SteadyStateSimulation::run, &m_sim);
}

void DialogSteadystate::update() {

  // update error plot
  std::shared_ptr<sme::simulate::SteadyStateData> data =
      m_sim.getLatestData().load();

  if (data != nullptr) {
    ui->errorPlot->graph(0)->addData(data->step, data->error);
    ui->errorPlot->graph(1)->addData(data->step, m_sim.getStopTolerance());
    ui->errorPlot->rescaleAxes(true);
    ui->errorPlot->replot(QCustomPlot::RefreshPriority::rpQueuedReplot);
  }
  // update image plot
  auto image = m_sim.getConcentrationImage(
      m_compartmentSpeciesToPlot,
      m_model.getDisplayOptions().normaliseOverAllSpecies);

  if (m_vizmode == VizMode::_2D) {
    ui->valuesPlot->setImage(image);
    ui->valuesPlot->setZIndex(ui->zaxis->value());
    ui->valuesPlot->repaint();
  } else {
    ui->valuesPlot3D->setImage(image);
    ui->valuesPlot3D->repaint();
  }
}

void DialogSteadystate::updateSpeciesToPlot() {
  m_compartmentSpeciesToPlot.clear();
  for (auto &&compSpecies : m_speciesNames) {
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
  SPDLOG_CRITICAL("finalise plots");
  m_plotRefreshTimer.stop();
  ui->btnReset->setEnabled(true);
}

////////////////////////////////////////////////////////////////////////////////
// lifecycle
DialogSteadystate::DialogSteadystate(sme::model::Model &model, QWidget *parent)
    : m_model(model), ui(std::make_unique<Ui::DialogSteadystate>()),
      m_sim(sme::simulate::SteadyStateSimulation(
          model, sme::simulate::SimulatorType::Pixel, 1e-6, 10,
          sme::simulate::SteadystateConvergenceMode::relative, 3600000, 1)),
      m_simulationFuture(), m_plotRefreshTimer(), m_vizmode(VizMode::_2D),
      m_isRunning(false) {

  SPDLOG_CRITICAL("construct dialog steadystate with solver: {}",
                  static_cast<int>(m_sim.getSimulatorType()));

  // set up bookkeeping data for display options
  m_compartmentSpeciesToPlot = m_sim.getCompartmentSpeciesIdxs();
  m_compartmentNames = m_model.getCompartments().getIds();
  m_speciesNames.resize(m_compartmentNames.size());
  for (std::size_t i = 0; i < m_compartmentNames.size(); ++i) {
    m_speciesNames[i] = m_model.getSpecies().getNames(m_compartmentNames[i]);
  }
  m_displayoptions = m_model.getDisplayOptions(); // get default display options

  // initialize ui elements and plotting panes
  initUi();
  initErrorPlot();
  initConcPlot();

  // connect slots and signals to make the ui functional
  connectSlots();

  SPDLOG_CRITICAL(
      " - m_speciesNames: {}",
      std::accumulate(m_speciesNames.begin(), m_speciesNames.end(),
                      std::string(""), [](const auto &a, const auto &b) {
                        std::string sublist = "[";
                        for (auto &&qstr : b) {
                          sublist = sublist + ", " + qstr.toStdString();
                        }
                        sublist += "], ";
                        return a + sublist;
                      }));
  SPDLOG_CRITICAL(
      " - m_compartmentSpeciesToPlot size: {}",
      std::accumulate(m_compartmentSpeciesToPlot.begin(),
                      m_compartmentSpeciesToPlot.end(), std::string(""),
                      [](const auto &a, const auto &b) {
                        std::string sublist = "[";
                        for (auto &&idx : b) {
                          sublist = sublist + ", " + std::to_string(idx);
                        }
                        sublist += "], ";
                        return a + sublist;
                      }));

  SPDLOG_CRITICAL("  - valuesPlot: {}, valuesPlot3D: {}",
                  ui->valuesPlot->isVisible(), ui->valuesPlot3D->isVisible());
  SPDLOG_CRITICAL(" - init plots done");
}

DialogSteadystate::~DialogSteadystate() = default;

////////////////////////////////////////////////////////////////////////////////
// slots
void DialogSteadystate::displayOptionsClicked() {
  DialogDisplayOptions dialog(m_compartmentNames, m_speciesNames,
                              m_model.getDisplayOptions(),
                              std::vector<PlotWrapperObservable>{}, this);

  // we don´t care about timetpoints here
  dialog.ui->cmbNormaliseOverAllTimepoints->hide();

  // we don´t care about adding observables here either for now. TODO: do we
  // really don´t care?
  dialog.ui->btnAddObservable->hide();
  dialog.ui->btnEditObservable->hide();
  dialog.ui->btnRemoveObservable->hide();
  dialog.ui->listObservables->hide();
  dialog.ui->chkShowMinMaxRanges->hide();

  if (dialog.exec() == QDialog::Accepted) {
    m_displayoptions.showMinMax = dialog.getShowMinMax();
    SPDLOG_CRITICAL("  currently selected species: {}", dialog.getShowSpecies(),
                    m_speciesNames.size());
    m_displayoptions.showSpecies = dialog.getShowSpecies();
    m_displayoptions.normaliseOverAllTimepoints = false;
    m_displayoptions.normaliseOverAllSpecies =
        dialog.getNormaliseOverAllSpecies();
    m_model.setDisplayOptions(m_displayoptions);
    updateSpeciesToPlot();

    // use this to wait for the dialog to finish updating before continuing
    // interval of 0 in the 'singleshot' calls immediatelly queues the update
    // function
    QCoreApplication::processEvents();
    QTimer::singleShot(0, this, &DialogSteadystate::update);
  }
}

void DialogSteadystate::plotUpdateTimerTimeout() {
  SPDLOG_DEBUG("refresh timer function");
  update();

  // check if simulation has converged or stopped early
  if (m_simulationFuture.valid() &&
      m_simulationFuture.wait_for(std::chrono::seconds(0)) ==
          std::future_status::ready) {

    if (m_sim.hasConverged()) {
      SPDLOG_CRITICAL("Simulation has converged: {}",
                      m_sim.getStepsBelowTolerance());
      m_isRunning = false;
      ui->btnStartStop->setText("Start");
      QMessageBox::information(this, "Simulation converged",
                               "The simulation has converged.");
      finalise();
    } else if (m_sim.getSolverStopRequested()) {
      m_isRunning = false;
      ui->btnStartStop->setText("Resume");
      SPDLOG_CRITICAL("Simulation has stopped early");
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
    }
  }
}

void DialogSteadystate::convergenceCurrentIndexChanged(int index) {
  SPDLOG_DEBUG("convergence clicked {}", index);

  if (index == 0) {
    m_sim.setConvergenceMode(
        sme::simulate::SteadystateConvergenceMode::absolute);
  } else {
    m_sim.setConvergenceMode(
        sme::simulate::SteadystateConvergenceMode::relative);
  }
}

void DialogSteadystate::plottingCurrentIndexChanged(
    [[maybe_unused]] int index) {
  SPDLOG_DEBUG("plotting clicked {}", index);

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
  QCoreApplication::processEvents();
  QTimer::singleShot(0, this, &DialogSteadystate::update);
  SPDLOG_CRITICAL(" - m_vizmode: {}", static_cast<int>(m_vizmode));
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
    m_isRunning = true;
    // start timer to periodically update simulation results
    ui->btnStartStop->setText("Stop");
    ui->btnReset->setEnabled(false);
    runAndPlot();
  } else {
    m_isRunning = false;
    m_sim.requestStop();

    if (m_simulationFuture.valid()) {
      SPDLOG_CRITICAL(" waiting for simulation to finish");
      m_simulationFuture.wait();
    }

    m_plotRefreshTimer.stop();

    if (m_sim.hasConverged()) {
      ui->btnStartStop->setText("Resume");
    } else {
      ui->btnStartStop->setText("Start");
    }
    finalise();
    QMessageBox::information(this, "Simulation stopped",
                             "The simulation has been stopped.");
  }
}

void DialogSteadystate::btnResetClicked() { reset(); }

void DialogSteadystate::btnOkClicked() {
  SPDLOG_DEBUG("ok clicked");
  finalise();
  accept();
}

void DialogSteadystate::btnCancelClicked() {
  SPDLOG_DEBUG("cancel clicked");
  finalise();
  reject();
}

void DialogSteadystate::zaxisValueChanged(int value) {
  SPDLOG_DEBUG("zaxis value changed to {}", value);
  ui->valuesPlot->setZIndex(value);
  QCoreApplication::processEvents();
  QTimer::singleShot(0, this, &DialogSteadystate::update);
}
