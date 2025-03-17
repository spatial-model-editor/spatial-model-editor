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
  connect(ui->diffMode, &QCheckBox::clicked, this,
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
  ui->diffMode->setCheckable(true);

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
  // handles the switch between 2D and 3D visualization modes
  // this option is always there, even if the data itself is not 3D
  SPDLOG_INFO("Mode index: {}", modeindex);
  if (modeindex == 0) {
    // 2D mode -> show the vizualization of the target and result images
    // that are still there
    vizMode = VizMode::_2D;
    ui->lblTarget->setVisible(true);
    ui->lblResult->setVisible(!differenceMode);
    ui->zaxis->setVisible(true);
    ui->lblTarget3D->setVisible(false);
    ui->lblResult3D->setVisible(false);
    updateTargetImage();
    updateResultImage();

  } else {

    // show the 3D visualization of the target and result images
    vizMode = VizMode::_3D;
    ui->lblTarget->setVisible(false);
    ui->lblResult->setVisible(false);
    ui->zaxis->setVisible(false);
    ui->lblTarget3D->setVisible(true);
    ui->lblResult3D->setVisible(!differenceMode);
    updateTargetImage();
    updateResultImage();
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
  // displays the naive difference of target and best result in the first
  // plot pane.

  SPDLOG_DEBUG("Difference mode clicked {}", differenceMode);

  if (m_opt == nullptr) {
    return;
  }

  differenceMode = !differenceMode;
  SPDLOG_DEBUG("Difference mode after: {}", differenceMode);

  ui->lblResult->setVisible(!differenceMode && vizMode == VizMode::_2D);
  ui->lblResult3D->setVisible(!differenceMode && vizMode == VizMode::_3D);
  ui->lblResultLabel->setVisible(!differenceMode);

  updateTargetImage();

  if (differenceMode) {

    ui->lblTargetLabel->setText(
        "Difference between estimated and target image:");
  } else {
    updateResultImage();
    // reset the images when the difference mode is turned off
    ui->lblTargetLabel->setText("Target values:");
  }
  SPDLOG_DEBUG("res label visible: {}", ui->lblResultLabel->isVisible());
  SPDLOG_DEBUG("2d res visible: {}", ui->lblResult->isVisible());
  SPDLOG_DEBUG("3d res visible: {}", ui->lblResult3D->isVisible());
  SPDLOG_DEBUG("2d tgt visible: {}", ui->lblTarget->isVisible());
  SPDLOG_DEBUG("3d tgt visible: {}", ui->lblTarget3D->isVisible());
  SPDLOG_DEBUG("tgt label: '{}'", ui->lblTargetLabel->text().toStdString());
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
    if (vizMode == VizMode::_2D) {
      ui->lblResult->setZSlider(ui->zaxis);
      ui->lblResult->setZIndex(ui->zaxis->value());
    }
  }

  if (differenceMode) {
    updateTargetImage();
  }
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

void DialogOptimize::updateTargetImage() {
  // this handles the distinction between 2D and 3D visualizations when setting
  // image data for the image that displays the target values
  // and handles the z-slider for the 2D visualization
  if (m_opt == nullptr) {
    return;
  }

  auto variable = ui->cmbTarget->currentIndex();
  SPDLOG_DEBUG(" Updating target image for variable {}", variable);

  auto img = differenceMode ? m_opt->getDifferenceImage(variable)
                            : m_opt->getTargetImage(variable);
  SPDLOG_DEBUG("difference mode: {}", differenceMode);
  SPDLOG_DEBUG(" data: {}, empty: {}", img.volume().depth(), img.empty());

  if (vizMode == VizMode::_2D) {
    SPDLOG_INFO("Updating target image for 2D visualization ");
    // all the z-axis stuff is only necessary for 2D visualization
    auto z = ui->zaxis->value();
    ui->lblTarget->setImage(img);
    ui->lblTarget->setZSlider(ui->zaxis);
    ui->lblTarget->setZIndex(z);
    ui->zaxis->setMaximum(ui->lblTarget->getImage().volume().depth());
    ui->zaxis->setValue(z);
    ui->lblTarget3D->updateGeometry();

  } else {
    SPDLOG_DEBUG("Updating target image for 3D visualization: 2D visible? {}, "
                 "3d visible? {}",
                 ui->lblTarget->isVisible(), ui->lblTarget3D->isVisible());
    ui->lblTarget3D->setImage(img);
    ui->lblTarget3D->updateGeometry();
  }
}

void DialogOptimize::updateResultImage() {
  // this handles the distinction between 2D and 3D visualizations

  if (m_opt == nullptr) {
    return;
  }

  if (vizMode == VizMode::_2D) {
    auto z = ui->zaxis->value();
    ui->lblResult->setImage(m_opt->getCurrentBestResultImage());
    ui->lblResult->setZSlider(ui->zaxis);
    ui->lblResult->setZIndex(z);
    ui->lblResult->updateGeometry();
    // ui->lblResult->parentWidget()->layout()->invalidate();
    // ui->lblResult->parentWidget()->layout()->activate();
    // ui->lblResult->parentWidget()->adjustSize();
  } else {
    SPDLOG_DEBUG("Updating result image for 3D visualization  2D visible? {}, "
                 "3d visible? {}",
                 ui->lblResult->isVisible(), ui->lblResult3D->isVisible());
    ui->lblResult3D->setImage(m_opt->getCurrentBestResultImage());
    ui->lblResult3D->updateGeometry();
    // ui->lblResult3D->parentWidget()->layout()->invalidate();
    // ui->lblResult3D->parentWidget()->layout()->activate();
    // ui->lblResult3D->parentWidget()->adjustSize();
  }
}

// getters
DialogOptimize::VizMode DialogOptimize::getVizMode() const { return vizMode; }

bool DialogOptimize::getDifferenceMode() const { return differenceMode; }
