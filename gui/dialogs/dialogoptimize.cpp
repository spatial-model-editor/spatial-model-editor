#include "dialogoptimize.hpp"
#include "dialogoptsetup.hpp"
#include "qlabelmousetracker.hpp"
#include "sme/logger.hpp"
#include "sme/optimize.hpp"
#include "sme/utils.hpp"
#include "ui_dialogoptimize.h"
#include <qcustomplot.h>

static void initFitnessPlot(QCustomPlot *plot) {
  if (plot->plotLayout()->rowCount() == 1) {
    plot->plotLayout()->insertRow(0);
    plot->plotLayout()->addElement(0, 0,
                                   new QCPTextElement(plot, "Best Fitness"));
    plot->setInteraction(QCP::iRangeDrag, true);
    plot->setInteraction(QCP::iRangeZoom, true);
    plot->legend->setVisible(true);
  }
  plot->clearGraphs();
  plot->yAxis->setScaleType(QCPAxis::stLogarithmic);
  auto *graphFitness{plot->addGraph()};
  graphFitness->setPen(sme::common::indexedColours()[0]);
  graphFitness->setScatterStyle(
      QCPScatterStyle(QCPScatterStyle::ScatterShape::ssDisc));
  graphFitness->setName("Fitness of best parameters");
  plot->rescaleAxes(true);
  plot->replot();
}

static void initParamsPlot(QCustomPlot *plot,
                           const std::vector<QString> &paramNames) {
  if (plot->plotLayout()->rowCount() == 1) {
    plot->plotLayout()->insertRow(0);
    plot->plotLayout()->addElement(0, 0,
                                   new QCPTextElement(plot, "Best Parameters"));
    plot->setInteraction(QCP::iRangeDrag, true);
    plot->setInteraction(QCP::iRangeZoom, true);
    plot->legend->setVisible(true);
  }
  plot->clearGraphs();
  plot->yAxis->setScaleType(QCPAxis::stLogarithmic);
  for (std::size_t i = 0; i < paramNames.size(); ++i) {
    auto *graphParams{plot->addGraph()};
    graphParams->setPen(sme::common::indexedColours()[i]);
    graphParams->setScatterStyle(
        QCPScatterStyle(QCPScatterStyle::ScatterShape::ssDisc));
    graphParams->setName(paramNames[i]);
  }
  plot->rescaleAxes(true);
  plot->replot();
}

DialogOptimize::DialogOptimize(sme::model::Model &model, QWidget *parent)
    : QDialog(parent), model{model},
      ui{std::make_unique<Ui::DialogOptimize>()} {
  ui->setupUi(this);
  ui->splitter->setSizes({1000, 1000});
  for (auto *lbl : {ui->lblTarget, ui->lblResult}) {
    lbl->invertYAxis(model.getDisplayOptions().invertYAxis);
    lbl->displayScale(model.getDisplayOptions().showGeometryScale);
    lbl->displayGrid(model.getDisplayOptions().showGeometryGrid);
    lbl->setPhysicalSize(model.getGeometry().getPhysicalSize(),
                         model.getUnits().getLength().name);
  }
  connect(ui->cmbTarget, &QComboBox::currentIndexChanged, this,
          &DialogOptimize::cmbTarget_currentIndexChanged);
  connect(ui->btnStartStop, &QPushButton::clicked, this,
          &DialogOptimize::btnStartStop_clicked);
  connect(ui->btnSetup, &QPushButton::clicked, this,
          &DialogOptimize::btnSetup_clicked);
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

QString DialogOptimize::getParametersString() const {
  QString s{};
  if (opt == nullptr || opt->getParams().empty()) {
    return s;
  }
  for (std::size_t i = 0; i < opt->getParamNames().size(); ++i) {
    s.append(opt->getParamNames()[i])
        .append(": ")
        .append(QString::number(opt->getParams().back()[i]));
    if (i < opt->getParamNames().size() - 1) {
      s.append("\n");
    }
  }
  return s;
}

void DialogOptimize::applyToModel() const {
  opt->applyParametersToModel(&model);
}

void DialogOptimize::init() {
  ui->lblResult->setImage({});
  ui->cmbTarget->clear();
  if (model.getOptimizeOptions().optParams.empty() ||
      model.getOptimizeOptions().optCosts.empty()) {
    opt.reset();
    ui->btnStartStop->setEnabled(false);
    ui->btnSetup->setEnabled(true);
    ui->cmbTarget->setEnabled(false);
    initFitnessPlot(ui->pltFitness);
    initParamsPlot(ui->pltParams, {});
    ui->lblTarget->setImage({});
    return;
  }
  ui->cmbTarget->setEnabled(true);
  for (const auto &optCost : model.getOptimizeOptions().optCosts) {
    ui->cmbTarget->addItem(optCost.name.c_str());
  }
  this->setCursor(Qt::WaitCursor);
  opt = std::make_unique<sme::simulate::Optimization>(model);
  this->setCursor(Qt::ArrowCursor);
  ui->btnStartStop->setEnabled(true);
  ui->btnSetup->setEnabled(true);
  ui->lblTarget->setImage(opt->getTargetImage(ui->cmbTarget->currentIndex()));
  initFitnessPlot(ui->pltFitness);
  initParamsPlot(ui->pltParams, opt->getParamNames());
}

void DialogOptimize::cmbTarget_currentIndexChanged(int index) {
  if (opt == nullptr || index < 0 || index >= ui->cmbTarget->count()) {
    ui->lblTarget->setImage({});
    return;
  }
  ui->lblTarget->setImage(opt->getTargetImage(static_cast<std::size_t>(index)));
  if (auto img{opt->getUpdatedBestResultImage(ui->cmbTarget->currentIndex())};
      img.has_value()) {
    ui->lblResult->setImage(img.value());
  }
}

void DialogOptimize::btnStartStop_clicked() {
  if (opt->getIsRunning()) {
    opt->requestStop();
    ui->btnStartStop->setText("Start");
    ui->btnStartStop->setEnabled(false);
    return;
  }
  ui->btnStartStop->setText("Stop");
  ui->btnSetup->setEnabled(false);
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

void DialogOptimize::btnSetup_clicked() {
  DialogOptSetup dialogOptSetup(model);
  if (dialogOptSetup.exec() == QDialog::Accepted) {
    model.getOptimizeOptions() = dialogOptSetup.getOptimizeOptions();
    init();
  }
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
  if (auto img{opt->getUpdatedBestResultImage(ui->cmbTarget->currentIndex())};
      img.has_value()) {
    ui->lblResult->setImage(img.value());
  }
}

void DialogOptimize::finalizePlots() {
  plotRefreshTimer.stop();
  ui->btnStartStop->setEnabled(true);
  ui->btnSetup->setEnabled(true);
  ui->buttonBox->setEnabled(true);
  updatePlots();
  this->setCursor(Qt::ArrowCursor);
  if (opt != nullptr && !opt->getErrorMessage().empty()) {
    QMessageBox::warning(this, "Optimize error",
                         opt->getErrorMessage().c_str());
    ui->btnStartStop->setText("Start");
  }
}
