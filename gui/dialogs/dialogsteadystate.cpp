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

  // set up gui update slot: update Gui every second
  m_plotRefreshTimer.setTimerType(Qt::TimerType::VeryCoarseTimer);
  m_plotRefreshTimer.setInterval(1000);
  connect(&m_plotRefreshTimer, &QTimer::timeout, this,
          &DialogSteadystate::plotUpdateTimerTimeout);
}

void DialogSteadystate::initPlots() {
  SPDLOG_CRITICAL("init plots");

  // init error plot
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

  // init image plots
  ui->valuesPlot->show();
  ui->valuesPlot->invertYAxis(m_model.getDisplayOptions().invertYAxis);
  ui->valuesPlot->displayScale(m_model.getDisplayOptions().showGeometryScale);
  ui->valuesPlot->displayGrid(m_model.getDisplayOptions().showGeometryGrid);
  ui->valuesPlot->setPhysicalUnits(m_model.getUnits().getLength().name);

  auto volume = m_model.getGeometry().getImages().volume();
  // manipulate UI to reduce clutter when solving 2D problem
  if (volume.depth() == 1) {
    SPDLOG_CRITICAL("2D problem detected");
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

  SPDLOG_CRITICAL(" - init plots done");
}

void DialogSteadystate::resetPlots() {
  ui->errorPlot->clearGraphs();
  ui->valuesPlot->setImage(sme::common::ImageStack{});
  ui->valuesPlot3D->setImage(sme::common::ImageStack{});

  initPlots();
}

void DialogSteadystate::reset() {
  SPDLOG_CRITICAL("reset");

  if (m_isRunning) {
    SPDLOG_CRITICAL("  cannot reset while simulation is running");
    QMessageBox::warning(this, "Reset",
                         "Cannot reset while simulation is running.");
    return;
  }
  m_sim.reset();
  SPDLOG_CRITICAL("sim values: stopTol: {}, stepsTilConvergence: {}, "
                  "stepsBelowTol: {}, timeout: {}",
                  m_sim.getStopTolerance(), m_sim.getStepsToConvergence(),
                  m_sim.getStepsBelowTolerance(), m_sim.getTimeout());
  ui->btnStartStop->setText("Start");
  m_isRunning = false;
  resetPlots();
}

void DialogSteadystate::runAndPlot() {
  SPDLOG_CRITICAL(" run and plot");
  m_plotRefreshTimer.start();

  /// start simulation in a different thread
  m_simulationFuture = std::async(
      std::launch::async, &sme::simulate::SteadyStateSimulation::run, &m_sim);
}

void DialogSteadystate::update() {
  SPDLOG_CRITICAL("  update UI");
  SPDLOG_CRITICAL("  - ui->valuesPlot->isVisible(): {}",
                  ui->valuesPlot->isVisible());
  SPDLOG_CRITICAL("  - ui->valuesPlot3D->isVisible(): {}",
                  ui->valuesPlot3D->isVisible());

  // update error plot
  sme::simulate::SteadyStateData data = *m_sim.getLatestData().load();
  SPDLOG_CRITICAL(" latest data: step: {}, error: {}, hasconverged? {}",
                  data.step, data.error, m_sim.hasConverged());
  ui->errorPlot->graph(0)->addData(data.step, data.error);
  ui->errorPlot->graph(1)->addData(data.step, m_sim.getStopTolerance());
  ui->errorPlot->rescaleAxes(true);
  ui->errorPlot->replot(QCustomPlot::RefreshPriority::rpQueuedReplot);

  // update image plot
  auto image = m_sim.getConcentrationImage(
      m_compartmentSpeciesToPlot,
      m_model.getDisplayOptions().normaliseOverAllSpecies);
  if (m_vizmode == VizMode::_2D) {
    SPDLOG_CRITICAL("update 2D plot");
    ui->valuesPlot->setImage(image);
    ui->valuesPlot->setZIndex(ui->zaxis->value());
    ui->valuesPlot->repaint();
  } else {
    SPDLOG_CRITICAL("update 3D plot");
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

  for (auto &&species : m_compartmentSpeciesToPlot) {
    for (auto &&s : species) {
      SPDLOG_CRITICAL("  - species to plot: {}", s);
    }
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
  for (auto &&compartment : m_model.getCompartments().getIds()) {
    m_compartmentNames.append(compartment);
    QStringList species;
    std::vector<std::size_t> speciesToPlot;
    for (int i = 0; i < m_model.getSpecies().getIds(compartment).size(); ++i) {
      species.append(m_model.getSpecies().getIds(compartment)[i]);
      speciesToPlot.push_back(i);
    }
    m_speciesNames.emplace_back(std::move(species));
    m_compartmentSpeciesToPlot.emplace_back(std::move(speciesToPlot));
  }

  initUi();
  connectSlots();
  initPlots();
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

  // we don´t care about adding observables  here either
  dialog.ui->btnAddObservable->hide();
  dialog.ui->btnEditObservable->hide();
  dialog.ui->btnRemoveObservable->hide();
  dialog.ui->listObservables->hide();
  dialog.ui->chkShowMinMaxRanges->hide();

  if (dialog.exec() == QDialog::Accepted) {
    m_displayoptions.showMinMax = dialog.getShowMinMax();
    SPDLOG_CRITICAL("  currently selected species: {}",
                    dialog.getShowSpecies());
    m_displayoptions.showSpecies = dialog.getShowSpecies();
    m_displayoptions.normaliseOverAllTimepoints = false;
    m_displayoptions.normaliseOverAllSpecies =
        dialog.getNormaliseOverAllSpecies();
    m_model.setDisplayOptions(m_displayoptions);
    updateSpeciesToPlot();
    update();
    finalise();
  }
}

void DialogSteadystate::plotUpdateTimerTimeout() {
  SPDLOG_CRITICAL("refresh timer function");
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
  if (index == 0) {
    m_sim.setConvergenceMode(
        sme::simulate::SteadystateConvergenceMode::absolute);
  } else {
    m_sim.setConvergenceMode(
        sme::simulate::SteadystateConvergenceMode::relative);
  }
  SPDLOG_CRITICAL("convergence clicked {}", index);
}

void DialogSteadystate::plottingCurrentIndexChanged(
    [[maybe_unused]] int index) {
  SPDLOG_CRITICAL("plotting clicked {}", index);

  if (m_vizmode == VizMode::_2D) {
    SPDLOG_CRITICAL("2D -> 3D");
    ui->valuesPlot->hide();
    ui->valuesPlot3D->show();
    ui->zaxis->hide();
    ui->zlabel->hide();
    m_vizmode = VizMode::_3D;
  } else {
    SPDLOG_CRITICAL("3D -> 2D");
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
  update();
  SPDLOG_CRITICAL(" - m_vizmode: {}", static_cast<int>(m_vizmode));
}

void DialogSteadystate::timeoutInputChanged() {
  // input is in seconds, but we store it in milliseconds
  m_sim.setTimeout(ui->timeoutInput->text().toDouble() * 1000);
  SPDLOG_CRITICAL("timeout change to {}, newly set: {}",
                  ui->timeoutInput->text().toDouble(), m_sim.getTimeout());
}

void DialogSteadystate::toleranceInputChanged() {
  m_sim.setStopTolerance(ui->toleranceInput->text().toDouble());
  SPDLOG_CRITICAL("tolerance changed to {}, newly set: {}",
                  ui->toleranceInput->text().toDouble(),
                  m_sim.getStopTolerance());
}

void DialogSteadystate::stepsWithinToleranceInputChanged() {
  m_sim.setStepsToConvergence(ui->tolStepInput->text().toInt());
  SPDLOG_CRITICAL("steps within tolerance changed to {}, newly set: {}",
                  ui->tolStepInput->text().toInt(),
                  m_sim.getStepsToConvergence());
}

void DialogSteadystate::convIntervalInputChanged() {
  m_sim.setDt(ui->convIntervalInput->text().toDouble());
  SPDLOG_CRITICAL("convergence interval changed to {}, newly set: {}",
                  ui->convIntervalInput->text().toDouble(), m_sim.getDt());
}

void DialogSteadystate::btnStartStopClicked() {
  SPDLOG_CRITICAL("start/stop clicked {}", m_isRunning);
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
  SPDLOG_CRITICAL("ok clicked");
  finalise();
  accept();
}

void DialogSteadystate::btnCancelClicked() {
  SPDLOG_CRITICAL("cancel clicked");
  finalise();
  reject();
}
