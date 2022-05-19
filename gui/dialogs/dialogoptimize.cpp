#include "dialogoptimize.hpp"
#include "dialogoptsetup.hpp"
#include "qlabelmousetracker.hpp"
#include "sme/logger.hpp"
#include "sme/optimize.hpp"
#include "sme/utils.hpp"
#include "ui_dialogoptimize.h"
#include <qcustomplot.h>

static void initFitnessPlot(QCustomPlot *plot, double initialFitness) {
  if (plot->plotLayout()->rowCount() == 1) {
    plot->plotLayout()->insertRow(0);
    plot->plotLayout()->addElement(0, 0,
                                   new QCPTextElement(plot, "Best Fitness"));
    plot->setInteraction(QCP::iRangeDrag, true);
    plot->setInteraction(QCP::iRangeZoom, true);
    plot->legend->setVisible(true);
  }
  plot->clearGraphs();
  auto *graphFitness{plot->addGraph()};
  graphFitness->setPen(sme::common::indexedColours()[0]);
  graphFitness->setScatterStyle(
      QCPScatterStyle(QCPScatterStyle::ScatterShape::ssDisc));
  graphFitness->setName("Fitness of best parameters");
  graphFitness->addData({0}, {initialFitness}, true);
  plot->rescaleAxes(true);
  plot->replot();
}

static void initParamsPlot(QCustomPlot *plot,
                           const std::vector<QString> &paramNames,
                           const std::vector<double> &params) {
  if (plot->plotLayout()->rowCount() == 1) {
    plot->plotLayout()->insertRow(0);
    plot->plotLayout()->addElement(0, 0,
                                   new QCPTextElement(plot, "Best Parameters"));
    plot->setInteraction(QCP::iRangeDrag, true);
    plot->setInteraction(QCP::iRangeZoom, true);
    plot->legend->setVisible(true);
  }
  plot->clearGraphs();
  for (std::size_t i = 0; i < params.size(); ++i) {
    auto *graphParams{plot->addGraph()};
    graphParams->setPen(sme::common::indexedColours()[i]);
    graphParams->setScatterStyle(
        QCPScatterStyle(QCPScatterStyle::ScatterShape::ssDisc));
    graphParams->setName(paramNames[i]);
    graphParams->addData({0}, {params[i]}, true);
  }
  plot->rescaleAxes(true);
  plot->replot();
}

DialogOptimize::DialogOptimize(
    sme::model::Model &model,
    const sme::simulate::OptimizeOptions &optimizeOptions, QWidget *parent)
    : QDialog(parent), model{model}, optimizeOptions{optimizeOptions},
      ui{std::make_unique<Ui::DialogOptimize>()} {
  ui->setupUi(this);
  connect(ui->btnStart, &QPushButton::clicked, this,
          &DialogOptimize::btnStart_clicked);
  connect(ui->btnStop, &QPushButton::clicked, this,
          &DialogOptimize::btnStop_clicked);
  connect(ui->btnSetup, &QPushButton::clicked, this,
          &DialogOptimize::btnSetup_clicked);
  connect(ui->btnApplyToModel, &QPushButton::clicked, this,
          &DialogOptimize::btnApplyToModel_clicked);
  connect(ui->buttonBox, &QDialogButtonBox::accepted, this,
          &DialogOptimize::accept);
  connect(ui->buttonBox, &QDialogButtonBox::rejected, this,
          &DialogOptimize::reject);
  init();
  plotRefreshTimer.setTimerType(Qt::TimerType::VeryCoarseTimer);
  constexpr int plotMsRefreshInterval{1000};
  plotRefreshTimer.setInterval(plotMsRefreshInterval);
  connect(&plotRefreshTimer, &QTimer::timeout, this, [this]() {
    updatePlots();
    if (!opt->getIsRunning()) {
      finalizePlots();
    }
  });
}

DialogOptimize::~DialogOptimize() = default;

void DialogOptimize::init() {
  if (optimizeOptions.optParams.empty() || optimizeOptions.optCosts.empty()) {
    opt.reset();
    ui->btnStart->setEnabled(false);
    ui->btnStop->setEnabled(false);
    ui->btnSetup->setEnabled(true);
    ui->btnApplyToModel->setEnabled(false);
    initFitnessPlot(ui->pltFitness, {});
    initParamsPlot(ui->pltParams, {}, {});
    return;
  }
  this->setCursor(Qt::WaitCursor);
  // todo: this constructor does one or more simulations, should be cancellable
  // & not blocking the GUI thread
  opt = std::make_unique<sme::simulate::Optimization>(model, optimizeOptions);
  this->setCursor(Qt::ArrowCursor);
  ui->btnStart->setEnabled(true);
  ui->btnStop->setEnabled(false);
  ui->btnSetup->setEnabled(true);
  ui->btnApplyToModel->setEnabled(false);
  initFitnessPlot(ui->pltFitness, opt->getFitness().front());
  initParamsPlot(ui->pltParams, opt->getParamNames(), opt->getParams().front());
}

void DialogOptimize::btnStart_clicked() {
  ui->btnStart->setEnabled(false);
  ui->btnStop->setEnabled(true);
  ui->btnSetup->setEnabled(false);
  ui->btnApplyToModel->setEnabled(false);
  ui->buttonBox->setEnabled(false);
  this->setCursor(Qt::WaitCursor);
  // start optimization in a new thread
  constexpr std::size_t nIterations{1024};
  optIterations =
      std::async(std::launch::async, &sme::simulate::Optimization::evolve,
                 opt.get(), nIterations);
  // start timer to periodically update simulation results
  plotRefreshTimer.start();
}

void DialogOptimize::btnStop_clicked() {
  opt->requestStop();
  ui->btnStop->setEnabled(false);
}

void DialogOptimize::btnSetup_clicked() {
  DialogOptSetup dialogOptSetup(model, optimizeOptions);
  if (dialogOptSetup.exec() == QDialog::Accepted) {
    optimizeOptions = dialogOptSetup.getOptimizeOptions();
    init();
  }
}

void DialogOptimize::btnApplyToModel_clicked() {
  opt->applyParametersToModel(&model);
}

void DialogOptimize::updatePlots() {
  if (opt == nullptr) {
    return;
  }
  std::size_t nAvailableIterations{opt->getIterations()};
  const auto &fitnesses{opt->getFitness()};
  const auto &params{opt->getParams()};
  for (std::size_t iIter = nPlottedIterations; iIter < nAvailableIterations;
       ++iIter) {
    SPDLOG_DEBUG("adding iteration {}", iIter);
    // process new fitness
    ui->pltFitness->graph(0)->addData({static_cast<double>(iIter)},
                                      {fitnesses[iIter]}, true);
    ui->pltFitness->rescaleAxes(true);
    ui->pltFitness->replot(QCustomPlot::RefreshPriority::rpQueuedReplot);
    // process new parameters
    for (std::size_t iParam = 0; iParam < params[iIter].size(); ++iParam) {
      ui->pltParams->graph(static_cast<int>(iParam))
          ->addData({static_cast<double>(iIter)}, {params[iIter][iParam]},
                    true);
    }
    ui->pltParams->rescaleAxes(true);
    ui->pltParams->replot(QCustomPlot::RefreshPriority::rpQueuedReplot);
  }
  nPlottedIterations = nAvailableIterations;
}

void DialogOptimize::finalizePlots() {
  plotRefreshTimer.stop();
  ui->btnStart->setEnabled(true);
  ui->btnStop->setEnabled(false);
  ui->btnSetup->setEnabled(true);
  ui->btnApplyToModel->setEnabled(true);
  ui->buttonBox->setEnabled(true);
  updatePlots();
  this->setCursor(Qt::ArrowCursor);
}
