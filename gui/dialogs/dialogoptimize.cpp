#include "dialogoptimize.hpp"
#include "dialogoptsetup.hpp"
#include "qlabelmousetracker.hpp"
#include "sme/logger.hpp"
#include "sme/optimize.hpp"
#include "sme/utils.hpp"
#include "ui_dialogoptimize.h"
#include <qcustomplot.h>
#include <qtabwidget.h>

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
  graphFitness->setPen(sme::common::indexedColors()[0]);
  graphFitness->setScatterStyle(
      QCPScatterStyle(QCPScatterStyle::ScatterShape::ssDisc));
  graphFitness->setName("Fitness of best parameters");
  plot->rescaleAxes(true);
  plot->replot();
}

static void initParamsPlot(QCustomPlot *plot,
                           const std::vector<std::string> &paramNames) {
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
    graphParams->setPen(sme::common::indexedColors()[i]);
    graphParams->setScatterStyle(
        QCPScatterStyle(QCPScatterStyle::ScatterShape::ssDisc));
    graphParams->setName(paramNames[i].c_str());
  }
  plot->rescaleAxes(true);
  plot->replot();
}

DialogOptimize::DialogOptimize(sme::model::Model &model, QWidget *parent)
    : QDialog(parent), m_model{model},
      ui{std::make_unique<Ui::DialogOptimize>()} {

  ui->setupUi(this);
  ui->splitter->setSizes({1000, 1000});
  for (auto *lbl : {ui->lblTarget, ui->lblResult}) {
    lbl->invertYAxis(model.getDisplayOptions().invertYAxis);
    lbl->displayScale(model.getDisplayOptions().showGeometryScale);
    lbl->displayGrid(model.getDisplayOptions().showGeometryGrid);
    lbl->setPhysicalUnits(model.getUnits().getLength().name);
  }
  connect(ui->cmbTarget, &QComboBox::currentIndexChanged, this,
          &DialogOptimize::cmbTarget_currentIndexChanged);
  connect(ui->cmbMode, &QComboBox::currentIndexChanged, this,
          &DialogOptimize::cmbMode_currentIndexChanged);
  connect(ui->btnStartStop, &QPushButton::clicked, this,
          &DialogOptimize::btnStartStop_clicked);
  connect(ui->btnSetup, &QPushButton::clicked, this,
          &DialogOptimize::btnSetup_clicked);
  connect(ui->buttonBox, &QDialogButtonBox::accepted, this,
          &DialogOptimize::accept);
  connect(ui->buttonBox, &QDialogButtonBox::rejected, this,
          &DialogOptimize::reject);
  connect(ui->tabWidget, &QTabWidget::currentChanged, this,
          &DialogOptimize::updateTabs);
  init();

  m_plotRefreshTimer.setTimerType(Qt::TimerType::VeryCoarseTimer);
  constexpr int plotMsRefreshInterval{1000};
  m_plotRefreshTimer.setInterval(plotMsRefreshInterval);
  connect(&m_plotRefreshTimer, &QTimer::timeout, this, [this]() {
    updatePlots();
    if (!m_opt->getIsRunning()) {
      finalizePlots();
    }
  });
  ui->lblResult3D->hide();
  ui->lblTarget3D->hide();
  ui->lblDifference3D->hide();
}

DialogOptimize::~DialogOptimize() = default;

QString DialogOptimize::getParametersString() const {
  QString s{};
  if (m_opt == nullptr || m_opt->getParams().empty()) {
    return s;
  }
  auto paramNames = m_opt->getParamNames();
  for (std::size_t i = 0; i < paramNames.size(); ++i) {
    s.append(paramNames[i])
        .append(": ")
        .append(QString::number(m_opt->getParams().back()[i]));
    if (i < paramNames.size() - 1) {
      s.append("\n");
    }
  }
  return s;
}

void DialogOptimize::applyToModel() const {
  m_opt->applyParametersToModel(&m_model);
}

void DialogOptimize::init() {
  ui->lblResult->setImage({});
  ui->lblDifference->setImage({});
  ui->lblResult3D->setImage({});
  ui->lblTarget3D->setImage({});
  ui->lblDifference3D->setImage({});
  ui->cmbTarget->clear();
  ui->cmbMode->clear();
  if (m_model.getOptimizeOptions().optParams.empty() ||
      m_model.getOptimizeOptions().optCosts.empty()) {
    m_opt.reset();
    ui->btnStartStop->setEnabled(false);
    ui->btnSetup->setEnabled(true);
    ui->cmbTarget->setEnabled(false);
    initFitnessPlot(ui->pltFitness);
    initParamsPlot(ui->pltParams, {});
    ui->lblTarget->setImage({});
    return;
  }
  ui->cmbTarget->setEnabled(true);
  for (const auto &optCost : m_model.getOptimizeOptions().optCosts) {
    ui->cmbTarget->addItem(optCost.name.c_str());
  }
  ui->cmbMode->setEnabled(true);
  for (auto &&label : {"2D", "3D"}) {
    ui->cmbMode->addItem(label);
  }

  this->setCursor(Qt::WaitCursor);
  m_opt = std::make_unique<sme::simulate::Optimization>(m_model);
  this->setCursor(Qt::ArrowCursor);
  ui->btnStartStop->setEnabled(true);
  ui->btnSetup->setEnabled(true);
  ui->lblTarget->setImage(m_opt->getTargetImage(
      static_cast<std::size_t>(ui->cmbTarget->currentIndex())));
  ui->lblDifference->setImage(m_opt->getDifferenceImage(
      static_cast<std::size_t>(ui->cmbTarget->currentIndex())));
  initFitnessPlot(ui->pltFitness);
  initParamsPlot(ui->pltParams, m_opt->getParamNames());

  // target and result images have the same dimensions, so we can use only one
  // to set the range of the z-slider
  ui->cmbMode->setCurrentIndex(0);
  ui->lblResult->setZSlider(ui->zaxisValues);
  ui->lblTarget->setZSlider(ui->zaxisValues);
  ui->lblDifference->setZSlider(ui->zaxisDifference);
  for (auto &&panel : {ui->lblTarget, ui->lblResult, ui->lblDifference}) {
    panel->setZIndex(0);
    panel->displayGrid(true);
    panel->displayScale(true);
  }
}

void DialogOptimize::cmbTarget_currentIndexChanged(int index) {
  if (m_opt == nullptr || index < 0 || index >= ui->cmbTarget->count()) {
    ui->lblTarget->setImage({});
    return;
  }
  ui->lblTarget->setImage(
      m_opt->getTargetImage(static_cast<std::size_t>(index)));
  if (auto img{m_opt->getUpdatedBestResultImage(
          static_cast<std::size_t>(ui->cmbTarget->currentIndex()))};
      img.has_value()) {
    ui->lblResult->setImage(img.value());
  }
}

void DialogOptimize::cmbMode_currentIndexChanged(int modeindex) {
  // handles the switch between 2D and 3D visualization modes
  // this option is always there, even if the data itself is not 3D
  SPDLOG_INFO("Mode index: {}", modeindex);

  vizMode = modeindex == 0 ? VizMode::_2D : VizMode::_3D;
  auto is3D = vizMode == VizMode::_3D;
  ui->lblTarget->setVisible(not is3D);
  ui->lblResult->setVisible(not is3D);
  ui->lblDifference->setVisible(not is3D);
  ui->zaxisDifference->setVisible(not is3D);
  ui->zlabelDifference->setVisible(not is3D);
  ui->zaxisValues->setVisible(not is3D);
  ui->zlabelValues->setVisible(not is3D);
  ui->lblTarget3D->setVisible(is3D);
  ui->lblResult3D->setVisible(is3D);
  ui->lblDifference3D->setVisible(is3D);
  updateTargetImage();
  updateResultImage();
  updateDifferenceImage();
  if (not is3D) {
  }
  SPDLOG_INFO("Viz mode: {}", static_cast<int>(vizMode));
}

void DialogOptimize::btnStartStop_clicked() {
  if (m_opt->getIsRunning()) {
    m_opt->requestStop();
    ui->btnStartStop->setText("Start");
    ui->btnStartStop->setEnabled(false);
    return;
  }
  ui->btnStartStop->setText("Stop");
  ui->btnSetup->setEnabled(false);
  ui->buttonBox->setEnabled(false);
  ui->splitter->setEnabled(false);
  this->setCursor(Qt::WaitCursor);
  // start optimization in a new thread
  constexpr std::size_t nIterations{1024};
  m_optIterations = std::async(
      std::launch::async, &sme::simulate::Optimization::evolve, m_opt.get(),
      nIterations, std::function<void(double, const std::vector<double> &)>{});
  // start timer to periodically update simulation results
  m_plotRefreshTimer.start();
}

void DialogOptimize::btnSetup_clicked() {
  DialogOptSetup dialogOptSetup(m_model);
  if (dialogOptSetup.exec() == QDialog::Accepted) {
    m_model.getOptimizeOptions() = dialogOptSetup.getOptimizeOptions();
    init();
  }
}

void DialogOptimize::updatePlots() {
  if (m_opt == nullptr) {
    return;
  }
  std::size_t nAvailableIterations{m_opt->getIterations()};
  const auto &fitnesses{m_opt->getFitness()};
  const auto &params{m_opt->getParams()};
  for (std::size_t iIter = m_nPlottedIterations; iIter < nAvailableIterations;
       ++iIter) {
    SPDLOG_INFO("adding iteration {}", iIter);
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
  m_nPlottedIterations = nAvailableIterations;

  if (auto img{m_opt->getUpdatedBestResultImage(
          static_cast<std::size_t>(ui->cmbTarget->currentIndex()))};
      img.has_value()) {
    ui->lblResult->setImage(img.value());
  }

  updateDifferenceImage();
}

void DialogOptimize::updateTabs() {
  if (ui->tabWidget->currentIndex() == 0) {
    updateTargetImage();
    updateResultImage();
  } else if (ui->tabWidget->currentIndex() == 1) {
    updateDifferenceImage();
  } else {
    // put further stuff here later
    SPDLOG_CRITICAL("Invalid tab index: {}", ui->tabWidget->currentIndex());
  }
}

void DialogOptimize::finalizePlots() {
  m_plotRefreshTimer.stop();
  ui->btnStartStop->setEnabled(true);
  ui->btnSetup->setEnabled(true);
  ui->buttonBox->setEnabled(true);
  ui->splitter->setEnabled(true);
  updatePlots();
  this->setCursor(Qt::ArrowCursor);
  if (m_opt != nullptr && !m_opt->getErrorMessage().empty()) {
    QMessageBox::warning(this, "Optimize error",
                         m_opt->getErrorMessage().c_str());
    ui->btnStartStop->setText("Start");
  }
}

void DialogOptimize::updateTargetImage() {
  // this handles the distinction between 2D and 3D visualizations when setting
  // image data for the image that displays the target values
  // and handles the z-slider for the 2D visualization
  auto variable = ui->cmbTarget->currentIndex();
  if (m_opt == nullptr || variable < 0 || variable >= ui->cmbTarget->count()) {
    return;
  }

  SPDLOG_DEBUG(" Updating target image for variable {}", variable);

  auto img = m_opt->getTargetImage(variable);
  SPDLOG_DEBUG(" data: {}, empty: {}", img.volume().depth(), img.empty());

  if (vizMode == VizMode::_2D) {
    SPDLOG_INFO("Updating target image for 2D visualization ");
    // all the z-axis stuff is only necessary for 2D visualization
    ui->lblTarget->setImage(img);

  } else {
    SPDLOG_DEBUG("Updating target image for 3D visualization: 2D visible? {}, "
                 "3d visible? {}",
                 ui->lblTarget->isVisible(), ui->lblTarget3D->isVisible());
    ui->lblTarget3D->setImage(img);
  }
}

void DialogOptimize::updateResultImage() {
  // this handles the distinction between 2D and 3D visualizations

  if (m_opt == nullptr) {
    return;
  }

  if (vizMode == VizMode::_2D) {
    ui->lblResult->setImage(m_opt->getCurrentBestResultImage());
  } else {
    SPDLOG_DEBUG("Updating result image for 3D visualization  2D visible? {}, "
                 "3d visible? {}",
                 ui->lblResult->isVisible(), ui->lblResult3D->isVisible());
    ui->lblResult3D->setImage(m_opt->getCurrentBestResultImage());
  }
}

void DialogOptimize::updateDifferenceImage() {
  auto variable = ui->cmbTarget->currentIndex();
  if (m_opt == nullptr || variable < 0 || variable >= ui->cmbTarget->count()) {
    return;
  }
  SPDLOG_DEBUG(" Updating difference image for variable {}", variable);
  auto img = m_opt->getDifferenceImage(variable);
  if (vizMode == VizMode::_2D) {
    ui->lblDifference->setImage(img);
  } else {
    ui->lblDifference3D->setImage(img);
  }
}

// getters
DialogOptimize::VizMode DialogOptimize::getVizMode() const { return vizMode; }
