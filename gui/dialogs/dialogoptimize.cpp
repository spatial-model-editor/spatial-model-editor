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
  graphFitness->setPen(sme::common::indexedColors()[0]);
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
    graphParams->setPen(sme::common::indexedColors()[i]);
    graphParams->setScatterStyle(
        QCPScatterStyle(QCPScatterStyle::ScatterShape::ssDisc));
    graphParams->setName(paramNames[i]);
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
  connect(ui->differenceMode, &QPushButton::clicked, this,
          &DialogOptimize::differenceMode_clicked);
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
}

DialogOptimize::~DialogOptimize() = default;

QString DialogOptimize::getParametersString() const {
  QString s{};
  if (m_opt == nullptr || m_opt->getParams().empty()) {
    return s;
  }
  for (std::size_t i = 0; i < m_opt->getParamNames().size(); ++i) {
    s.append(m_opt->getParamNames()[i])
        .append(": ")
        .append(QString::number(m_opt->getParams().back()[i]));
    if (i < m_opt->getParamNames().size() - 1) {
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
  ui->cmbTarget->clear();
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
  ui->cmbMode->clear();
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
  initFitnessPlot(ui->pltFitness);
  initParamsPlot(ui->pltParams, m_opt->getParamNames());

  ui->lblResult->setZSlider(ui->zaxis);
  ui->lblTarget->setZSlider(ui->zaxis);
  ui->lblResult->setZIndex(0);
  ui->lblTarget->setZIndex(0);
  ui->lblTarget->displayGrid(true);
  ui->lblTarget->displayScale(true);
  ui->lblResult->displayGrid(true);
  ui->lblResult->displayScale(true);

  // target and result images have the same dimensions, so we can use only one
  // to set the range of the z-slider
  ui->zaxis->setMinimum(0);
  ui->zaxis->setMaximum(m_opt
                            ->getTargetImage(static_cast<std::size_t>(
                                ui->cmbTarget->currentIndex()))
                            .volume()
                            .depth());
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
  // TODO
  SPDLOG_INFO("Mode index: {}", modeindex);
  if (modeindex == 0) {
    vizMode = VizMode::_2D;
    ui->lblTarget->setVisible(true);
    ui->lblResult->setVisible(true);
  } else {
    vizMode = VizMode::_3D;
    // README: this is temporary and only there to show an effect of setting the
    // dim toggle.
    ui->lblTarget->setVisible(false);
    ui->lblResult->setVisible(false);
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
  this->setCursor(Qt::WaitCursor);
  // start optimization in a new thread
  constexpr std::size_t nIterations{1024};
  m_optIterations =
      std::async(std::launch::async, &sme::simulate::Optimization::evolve,
                 m_opt.get(), nIterations);
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

void DialogOptimize::differenceMode_clicked() {
  // this function is the slot connected to the clicked signal of the
  // differenceMode checkbox. disables the second plot pane for the result and
  // displays the absolute difference of best result and target in the first
  // pane. The z-axis slider is used to select the z-index of the displayed
  // image.
  if (m_opt == nullptr) {
    return;
  }

  differenceMode = !differenceMode;
  SPDLOG_INFO("Difference mode: {}", differenceMode);
  ui->lblResult->setVisible(!differenceMode);
  ui->lblResultLabel->setVisible(!differenceMode);

  auto variable = ui->cmbTarget->currentIndex();
  auto z = ui->zaxis->value();

  if (differenceMode) {
    auto diff = m_opt->getDifferenceImage(variable);
    ui->lblTarget->setImage(diff);
  } else {

    // reset the images when the difference mode is turned off
    auto tgt = m_opt->getTargetImage(variable);
    ui->lblTarget->setImage(tgt);
    ui->lblResult->setImage(m_opt->getCurrentBestResultImage());
  }
  ui->lblTargetLabel->setText(
      differenceMode ? "Difference between estimated and target image:"
                     : "Target values:");

  ui->lblTarget->setZSlider(ui->zaxis);
  ui->lblTarget->setZIndex(z);
  ui->lblResult->setZSlider(ui->zaxis);
  ui->lblResult->setZIndex(z);
  ui->zaxis->setMaximum(ui->lblTarget->getImage().volume().depth());
  ui->zaxis->setValue(z);
  // Force the layout to update. This is intended to fix the problem where the
  // hidden image is resized to its minimum after its visibility is toggled.
  // TODO: is there a more straight forward way to do this?
  ui->lblResult->updateGeometry();
  ui->lblResult->parentWidget()->layout()->invalidate();
  ui->lblResult->parentWidget()->layout()->activate();
  ui->lblResult->parentWidget()->adjustSize();
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
    ui->lblResult->setZSlider(ui->zaxis);
    ui->lblResult->setZIndex(ui->zaxis->value());
  }

  // TODO: update target image in the lblTarget widget if differenceMode is true
  // and respect z-index
}

void DialogOptimize::finalizePlots() {
  m_plotRefreshTimer.stop();
  ui->btnStartStop->setEnabled(true);
  ui->btnSetup->setEnabled(true);
  ui->buttonBox->setEnabled(true);
  updatePlots();
  this->setCursor(Qt::ArrowCursor);
  if (m_opt != nullptr && !m_opt->getErrorMessage().empty()) {
    QMessageBox::warning(this, "Optimize error",
                         m_opt->getErrorMessage().c_str());
    ui->btnStartStop->setText("Start");
  }
}
