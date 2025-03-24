#include "dialogsteadystate.hpp"
#include "ui_dialogsteadystate.h"
#include <memory>

DialogSteadystate::DialogSteadystate(sme::model::Model &model, QWidget *parent)
    : ui(std::make_unique<Ui::DialogSteadystate>()) {
  ui->setupUi(this);
  ui->splitter->setSizes({1000, 1000});
  // for (auto *lbl : {ui->lblTarget, ui->lblResult}) {
  //   lbl->invertYAxis(model.getDisplayOptions().invertYAxis);
  //   lbl->displayScale(model.getDisplayOptions().showGeometryScale);
  //   lbl->displayGrid(model.getDisplayOptions().showGeometryGrid);
  //   lbl->setPhysicalUnits(model.getUnits().getLength().name);
  // }

  // connect all the shit here

  // connect(ui->cmbTarget, &QComboBox::currentIndexChanged, this,
  //         &DialogOptimize::cmbTarget_currentIndexChanged);
  // connect(ui->cmbMode, &QComboBox::currentIndexChanged, this,
  //         &DialogOptimize::cmbMode_currentIndexChanged);
  // connect(ui->btnStartStop, &QPushButton::clicked, this,
  //         &DialogOptimize::btnStartStop_clicked);
  // connect(ui->btnSetup, &QPushButton::clicked, this,
  //         &DialogOptimize::btnSetup_clicked);
  // connect(ui->buttonBox, &QDialogButtonBox::accepted, this,
  //         &DialogOptimize::accept);
  // connect(ui->buttonBox, &QDialogButtonBox::rejected, this,
  //         &DialogOptimize::reject);
  // connect(ui->diffMode, &QCheckBox::clicked, this,
  //         &DialogOptimize::differenceMode_clicked);

  // this has been taken from dialogoptimize - god knows what kind of shit this
  // is doing
  init();
  m_plotRefreshTimer.setTimerType(Qt::TimerType::VeryCoarseTimer);
  constexpr int plotMsRefreshInterval{1000};
  m_plotRefreshTimer.setInterval(plotMsRefreshInterval);
  connect(&m_plotRefreshTimer, &QTimer::timeout, this, [this]() {
    updatePlots();
    // if (!m_opt->getIsRunning()) {
    //   finalizePlots();
    // }
  });
}

void DialogSteadystate::init() {}

void DialogSteadystate::solver_currentIndexChanged(int index) {}

void DialogSteadystate::convergenceMode_currentIndexChanged(int index) {}

void DialogSteadystate::plottingMode_currentIndexChanged(int index) {}

void DialogSteadystate::timeout_input() {}

void DialogSteadystate::tolerance_input() {}

void DialogSteadystate::btnStartStop_clicked() {}

void DialogSteadystate::updatePlots() {}

void DialogSteadystate::finalizePlots() {}

void DialogSteadystate::selectZ() {}
