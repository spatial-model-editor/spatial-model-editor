#include "dialogsteadystate.hpp"
#include "sme/utils.hpp"
#include "ui_dialogsteadystate.h"

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
// lifecycle
DialogSteadystate::DialogSteadystate(sme::model::Model &model, QWidget *parent)
    : m_model(model), ui(std::make_unique<Ui::DialogSteadystate>()),
      sim(sme::simulate::SteadyStateSimulation(
          model, sme::simulate::SimulatorType::Pixel, 1e-6, 10,
          sme::simulate::SteadystateConvergenceMode::relative, 3600000, 1)),
      m_simulationFuture(), m_plotRefreshTimer(), vizmode(VizMode::_2D),
      isRunning(false) {

  SPDLOG_CRITICAL("construct dialog steadystate with solver: {}",
                  static_cast<int>(sim.getSimulatorType()));

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
  ui->timeoutInput->setText(QString::number(sim.getTimeout() / 1000));

  ui->toleranceInput->setValidator(
      new QDoubleValidator(0, 1000000000, 2, this));
  ui->toleranceInput->setText(QString::number(sim.getStopTolerance()));

  ui->tolStepInput->setValidator(
      new QIntValidator(0, std::numeric_limits<int>::max(), this));
  ui->tolStepInput->setText(QString::number(sim.getStepsToConvergence()));

  ui->convIntervalInput->setValidator(
      new QDoubleValidator(0, std::numeric_limits<double>::max(), 2, this));
  ui->convIntervalInput->setText(QString::number(sim.getDt()));

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

  connect(ui->buttonBox, &QDialogButtonBox::accepted, this,
          &DialogSteadystate::btnOkClicked);

  connect(ui->buttonBox, &QDialogButtonBox::rejected, this,
          &DialogSteadystate::btnCancelClicked);

  // set up gui update slot: update Gui every second
  m_plotRefreshTimer.setTimerType(Qt::TimerType::VeryCoarseTimer);
  m_plotRefreshTimer.setInterval(1000);
  connect(&m_plotRefreshTimer, &QTimer::timeout, this, [this]() {
    SPDLOG_CRITICAL("refresh timer function");
    update();

    // check if simulation has converged or stopped early
    if (m_simulationFuture.valid() &&
        m_simulationFuture.wait_for(std::chrono::seconds(0)) ==
            std::future_status::ready) {

      if (sim.hasConverged()) {
        isRunning = false;
        SPDLOG_CRITICAL("Simulation has converged: {}",
                        sim.getStepsBelowTolerance());
        QMessageBox::information(this, "Simulation converged",
                                 "The simulation has converged.");
      } else if (sim.getSolverStopRequested()) {
        isRunning = false;
        SPDLOG_CRITICAL("Simulation has stopped early");
        if (sim.getSolverErrormessage() == "Simulation timed out") {
          QMessageBox::critical(this, "Simulation timed out",
                                sim.getSolverErrormessage().c_str());
        } else {
          QMessageBox::critical(this, "Simulation stopped early",
                                sim.getSolverErrormessage().c_str());
        }
      } else {
        // do nothing
      }
    }

    if (!isRunning) {
      ui->btnStartStop->setText("Start");
      finalize();
    }
  });
}

DialogSteadystate::~DialogSteadystate() = default;

////////////////////////////////////////////////////////////////////////////////
// slots
void DialogSteadystate::convergenceCurrentIndexChanged(int index) {
  if (index == 0) {
    sim.setConvergenceMode(sme::simulate::SteadystateConvergenceMode::absolute);
  } else {
    sim.setConvergenceMode(sme::simulate::SteadystateConvergenceMode::relative);
  }
  SPDLOG_CRITICAL("convergence clicked {}", index);
}

void DialogSteadystate::plottingCurrentIndexChanged(
    [[maybe_unused]] int index) {
  SPDLOG_CRITICAL("plotting clicked {}", index);

  if (vizmode == VizMode::_2D) {
    SPDLOG_CRITICAL("2D -> 3D");
    ui->valuesPlot->hide();
    ui->valuesPlot3D->show();
    ui->zaxis->hide();
    ui->zlabel->hide();
    vizmode = VizMode::_3D;
  } else {
    SPDLOG_CRITICAL("3D -> 2D");
    ui->valuesPlot->show();
    ui->valuesPlot3D->hide();
    ui->zaxis->show();
    ui->zlabel->show();
    vizmode = VizMode::_2D;
  }
  SPDLOG_CRITICAL(" - vizmode: {}", static_cast<int>(vizmode));
}

void DialogSteadystate::timeoutInputChanged() {
  // input is in seconds, but we store it in milliseconds
  sim.setTimeout(ui->timeoutInput->text().toDouble() * 1000);
  SPDLOG_CRITICAL("timeout change to {}, newly set: {}",
                  ui->timeoutInput->text().toDouble(), sim.getTimeout());
}

void DialogSteadystate::toleranceInputChanged() {
  sim.setStopTolerance(ui->toleranceInput->text().toDouble());
  SPDLOG_CRITICAL("tolerance changed to {}, newly set: {}",
                  ui->toleranceInput->text().toDouble(),
                  sim.getStopTolerance());
}

void DialogSteadystate::stepsWithinToleranceInputChanged() {
  sim.setStepsToConvergence(ui->tolStepInput->text().toInt());
  SPDLOG_CRITICAL("steps within tolerance changed to {}, newly set: {}",
                  ui->tolStepInput->text().toInt(),
                  sim.getStepsToConvergence());
}

void DialogSteadystate::convIntervalInputChanged() {
  sim.setDt(ui->convIntervalInput->text().toDouble());
  SPDLOG_CRITICAL("convergence interval changed to {}, newly set: {}",
                  ui->convIntervalInput->text().toDouble(), sim.getDt());
}

void DialogSteadystate::btnStartStopClicked() {
  SPDLOG_CRITICAL("start/stop clicked {}", isRunning);
  if (!isRunning) {
    SPDLOG_CRITICAL("run the thing");
    isRunning = true;
    // start timer to periodically update simulation results
    ui->btnStartStop->setText("Stop");
    runAndPlot();
  } else {
    SPDLOG_CRITICAL("stop the thing");
    isRunning = false;
    sim.requestStop();

    if (m_simulationFuture.valid()) {
      SPDLOG_CRITICAL(" waiting for simulation to finish");
      m_simulationFuture.wait();
    }
    m_plotRefreshTimer.stop();
    finalize();
    ui->btnStartStop->setText("Start");
    QMessageBox::information(this, "Simulation stopped",
                             "The simulation has been stopped.");
  }
}

void DialogSteadystate::btnResetClicked() { reset(); }

void DialogSteadystate::btnOkClicked() {
  SPDLOG_CRITICAL("ok clicked");
  finalize();
  accept();
}

void DialogSteadystate::btnCancelClicked() {
  SPDLOG_CRITICAL("cancel clicked");
  finalize();
  reject();
}

////////////////////////////////////////////////////////////////////////////////
// helper functions

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

  ui->valuesPlot3D->hide();

  SPDLOG_CRITICAL(" - init plots done");
}

void DialogSteadystate::resetPlots() {
  ui->errorPlot->clearGraphs();
  ui->valuesPlot->clear();

  initPlots();
}

void DialogSteadystate::reset() {
  SPDLOG_CRITICAL("reset");
  sim.reset();
  SPDLOG_CRITICAL("sim values: stopTol: {}, stepsTilConvergence: {}, "
                  "stepsBelowTol: {}, timeout: {}",
                  sim.getStopTolerance(), sim.getStepsToConvergence(),
                  sim.getStepsBelowTolerance(), sim.getTimeout());
  ui->btnStartStop->setText("Start");
  isRunning = false;
  resetPlots();
}

void DialogSteadystate::runAndPlot() {
  SPDLOG_CRITICAL(" run and plot");
  resetPlots();
  m_plotRefreshTimer.start();

  /// start simulation in a different thread
  m_simulationFuture = std::async(
      std::launch::async, &sme::simulate::SteadyStateSimulation::run, &sim);
}

void DialogSteadystate::update() {
  SPDLOG_CRITICAL("  update UI");

  // update error plot
  sme::simulate::SteadyStateData data = *sim.getLatestData().load();
  SPDLOG_CRITICAL(" latest data: step: {}, error: {}", data.step, data.error);
  ui->errorPlot->graph(0)->addData(data.step, data.error);
  ui->errorPlot->graph(1)->addData(data.step, sim.getStopTolerance());
  ui->errorPlot->rescaleAxes(true);
  ui->errorPlot->replot(QCustomPlot::RefreshPriority::rpQueuedReplot);

  // update image plot
  if (vizmode == VizMode::_2D) {
    SPDLOG_CRITICAL("update 2D plot");
    ui->valuesPlot->setImage(data.concentrationImageStack);
    ui->valuesPlot->setZIndex(ui->zaxis->value());
    ui->valuesPlot->repaint();
  } else {
    SPDLOG_CRITICAL("update 3D plot");
    ui->valuesPlot3D->setImage(data.concentrationImageStack);
    ui->valuesPlot->repaint();
  }
}

void DialogSteadystate::finalize() {
  SPDLOG_CRITICAL("finalize plots");
  m_plotRefreshTimer.stop();
  // TODO: implement this bs
}

void DialogSteadystate::selectZ() {
  SPDLOG_CRITICAL("select z");
  // TODO: implement this bs
}
