#include "dialogsteadystate.hpp"
#include "dialogoptimize.hpp"
#include "ui_dialogsteadystate.h"
#include <memory>
#include <qcombobox.h>
#include <qline.h>
#include <qlineedit.h>
#include <qvalidator.h>

DialogSteadystate::DialogSteadystate(sme::model::Model &model, QWidget *parent)
    : ui(std::make_unique<Ui::DialogSteadystate>()), m_model(model) {

  // set up ui elements
  ui->setupUi(this);
  ui->splitter->setSizes({1000, 1000});

  ui->valuesPlot->invertYAxis(model.getDisplayOptions().invertYAxis);
  ui->valuesPlot->displayScale(model.getDisplayOptions().showGeometryScale);
  ui->valuesPlot->displayGrid(model.getDisplayOptions().showGeometryGrid);
  ui->valuesPlot->setPhysicalUnits(model.getUnits().getLength().name);

  ui->errorPlot->xAxis->setLabel("Steps");
  ui->errorPlot->yAxis->setLabel("Error");

  // add options ot the shitty combo boxes
  ui->cmbSolver->addItems({"Pixel (FDM)", "Dune (FEM)"});

  ui->cmbConvergence->addItems({"Absolute", "Relative"});

  ui->cmbPlotting->addItems({"2D", "3D"});

  m_plotRefreshTimer.setTimerType(Qt::TimerType::VeryCoarseTimer);

  constexpr int plotMsRefreshInterval{1000};

  m_plotRefreshTimer.setInterval(plotMsRefreshInterval);

  // set validators for input to only allow certain types of input
  ui->timeoutInput->setValidator(new QDoubleValidator(0, 1000000000, 2, this));

  ui->toleranceInput->setValidator(
      new QDoubleValidator(0, 1000000000, 2, this));

  ui->tolStepInput->setValidator(new QIntValidator(0, 1000000000, this));

  ui->maxStepInput->setValidator(new QIntValidator(0, 1000000000, this));

  // connect slots and signals
  connect(&m_plotRefreshTimer, &QTimer::timeout, this,
          [this]() { updatePlots(); });

  connect(ui->cmbSolver, &QComboBox::currentIndexChanged, this,
          &DialogSteadystate::solverCurrentIndexChanged);

  connect(ui->cmbConvergence, &QComboBox::currentIndexChanged, this,
          &DialogSteadystate::convergenceCurrentIndexChanged);

  connect(ui->cmbPlotting, &QComboBox::currentIndexChanged, this,
          &DialogSteadystate::plottingCurrentIndexChanged);

  connect(ui->timeoutInput, &QLineEdit::editingFinished, this,
          &DialogSteadystate::timeoutInputChanged);

  connect(ui->toleranceInput, &QLineEdit::editingFinished, this,
          &DialogSteadystate::toleranceInputChanged);

  connect(ui->tolStepInput, &QLineEdit::editingFinished, this,
          &DialogSteadystate::stepsWithinToleranceInputChanged);

  make_simulator();
  init_plots();
}

DialogSteadystate::~DialogSteadystate() = default;

// slots
void DialogSteadystate::solverCurrentIndexChanged(int index) {
  SPDLOG_CRITICAL("solver  clicked {}", index);

  if (index == 1) {
    sim.setSimulatorType(sme::simulate::SimulatorType::Dune);
  } else {
    sim.setSimulatorType(sme::simulate::SimulatorType::Pixel);
  }
  reset();
  init_plots();
}

void DialogSteadystate::init_plots() { SPDLOG_CRITICAL("init plots"); }

void DialogSteadystate::make_simulator() { SPDLOG_CRITICAL("make simulator"); }

void DialogSteadystate::reset() { SPDLOG_CRITICAL("reset"); }

void DialogSteadystate::convergenceCurrentIndexChanged(int index) {

  if (index == 0) {
    sim.setConvergenceMode(sme::simulate::SteadystateConvergenceMode::Absolute);
  } else {
    sim.setConvergenceMode(sme::simulate::SteadystateConvergenceMode::Relative);
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
    vizmode = VizMode::_3D;
  } else {
    SPDLOG_CRITICAL("3D -> 2D");
    ui->valuesPlot->show();
    ui->valuesPlot3D->hide();
    vizmode = VizMode::_2D;
  }
  updatePlots();
}

void DialogSteadystate::timeoutInputChanged() {
  sim.setTimeout(ui->timeoutInput->text().toDouble());
  SPDLOG_CRITICAL("timeout change to {}", ui->timeoutInput->text().toDouble());
}

void DialogSteadystate::toleranceInputChanged() {
  sim.setTolerance(ui->toleranceInput->text().toDouble());
  SPDLOG_CRITICAL("tolerance changed to {}",
                  ui->toleranceInput->text().toDouble());
}

void DialogSteadystate::stepsWithinToleranceInputChanged() {
  SPDLOG_CRITICAL("steps within tolerance changed to {}",
                  ui->tolStepInput->text().toInt());
  sim.setStepsWithinTolerance(ui->tolStepInput->text().toInt());
}

void DialogSteadystate::btnStartStopClicked() {
  SPDLOG_CRITICAL("start/stop clicked");
  // TODO: implement this bs
}

void DialogSteadystate::updatePlots() {
  SPDLOG_CRITICAL("update plots");
  // TODO: implement this bs
}

void DialogSteadystate::finalizePlots() {
  SPDLOG_CRITICAL("finalize plots");
  // TODO: implement this bs
}

void DialogSteadystate::selectZ() {
  SPDLOG_CRITICAL("select z");
  // TODO: implement this bs
}
