#include "tabsimulate.hpp"
#include "dialogexport.hpp"
#include "dialogimageslice.hpp"
#include "guiutils.hpp"
#include "logger.hpp"
#include "mesh.hpp"
#include "model.hpp"
#include "qlabelmousetracker.hpp"
#include "ui_tabsimulate.h"
#include "utils.hpp"
#include <QElapsedTimer>
#include <QMessageBox>
#include <algorithm>

TabSimulate::TabSimulate(model::Model &doc, QLabelMouseTracker *mouseTracker,
                         QWidget *parent)
    : QWidget(parent), ui{std::make_unique<Ui::TabSimulate>()}, sbmlDoc(doc),
      lblGeometry(mouseTracker), plt{std::make_unique<PlotWrapper>(
                                     "Average Concentration", this)} {
  ui->setupUi(this);
  ui->gridSimulate->addWidget(plt->plot, 1, 0, 1, 8);

  connect(ui->btnSimulate, &QPushButton::clicked, this,
          &TabSimulate::btnSimulate_clicked);
  connect(ui->btnResetSimulation, &QPushButton::clicked, this,
          &TabSimulate::loadModelData);
  connect(plt->plot, &QCustomPlot::mousePress, this,
          &TabSimulate::graphClicked);
  connect(ui->hslideTime, &QSlider::valueChanged, this,
          &TabSimulate::hslideTime_valueChanged);
  connect(ui->btnStopSimulation, &QPushButton::clicked, this,
          &TabSimulate::stopSimulation);
  connect(ui->btnSliceImage, &QPushButton::clicked, this,
          &TabSimulate::btnSliceImage_clicked);
  connect(ui->btnExport, &QPushButton::clicked, this,
          &TabSimulate::btnExport_clicked);
  connect(ui->btnDisplayOptions, &QPushButton::clicked, this,
          &TabSimulate::btnDisplayOptions_clicked);

  useDune(true);
  ui->hslideTime->setEnabled(false);
}

TabSimulate::~TabSimulate() = default;

void TabSimulate::loadModelData() {
  reset();
  if (!(sbmlDoc.getIsValid() && sbmlDoc.getGeometry().getIsValid())) {
    ui->hslideTime->setEnabled(false);
    ui->btnSimulate->setEnabled(false);
    return;
  }
  if (simType == simulate::SimulatorType::DUNE &&
      (sbmlDoc.getGeometry().getMesh() == nullptr ||
       !sbmlDoc.getGeometry().getMesh()->isValid())) {
    ui->btnSimulate->setEnabled(false);
    auto msgbox = newYesNoMessageBox(
        "Invalid mesh",
        "Mesh geometry is not valid, and is required for a DUNE "
        "simulation. Would you like to use the Pixel simulator instead?",
        this);
    connect(msgbox, &QMessageBox::finished, this, [this](int result) {
      if (result == QMessageBox::Yes) {
        useDune(false);
      }
    });
    msgbox->open();
    return;
  }
  sim = std::make_unique<simulate::Simulation>(sbmlDoc, simType, simOptions);
  if (!sim->errorMessage().empty()) {
    QMessageBox::warning(
        this, "Simulation Setup Failed",
        QString("Simulation setup failed.\n\nError message: %1")
            .arg(sim->errorMessage().c_str()));
    ui->btnSimulate->setEnabled(false);
    return;
  }

  ui->btnSimulate->setEnabled(true);

  // setup species names
  speciesNames.clear();
  compartmentNames.clear();
  for (std::size_t ic = 0; ic < sim->getCompartmentIds().size(); ++ic) {
    compartmentNames.push_back(
        sbmlDoc.getCompartments().getNames()[static_cast<int>(ic)]);
    auto &names = speciesNames.emplace_back();
    for (std::size_t is = 0; is < sim->getSpeciesIds(ic).size(); ++is) {
      names.push_back(
          sbmlDoc.getSpecies().getName(sim->getSpeciesIds(ic)[is].c_str()));
    }
  }
  // setup plot
  plt->plot->xAxis->setLabel(
      QString("time (%1)").arg(sbmlDoc.getUnits().getTime().name));
  plt->plot->yAxis->setLabel(
      QString("concentration (%1)").arg(sbmlDoc.getUnits().getConcentration()));
  plt->plot->xAxis->setRange(0, ui->txtSimLength->text().toDouble());
  // add lines
  for (std::size_t ic = 0; ic < sim->getCompartmentIds().size(); ++ic) {
    for (std::size_t is = 0; is < sim->getSpeciesIds(ic).size(); ++is) {
      QColor col = sim->getSpeciesColors(ic)[is];
      QString name =
          sbmlDoc.getSpecies().getName(sim->getSpeciesIds(ic)[is].c_str());
      plt->addAvMinMaxLine(name, col);
    }
  }

  time.push_back(0);
  // get initial concentrations
  int speciesIndex = 0;
  for (std::size_t ic = 0; ic < sim->getCompartmentIds().size(); ++ic) {
    for (std::size_t is = 0; is < sim->getSpeciesIds(ic).size(); ++is) {
      auto conc = sim->getAvgMinMax(0, ic, is);
      plt->addAvMinMaxPoint(speciesIndex, 0.0, conc);
      ++speciesIndex;
    }
  }
  displayOptions = sbmlDoc.getDisplayOptions();
  if (auto ns{static_cast<std::size_t>(speciesIndex)};
      ns != displayOptions.showSpecies.size()) {
    // show species count doesn't match actual number of species
    // user probably added/removed species to model
    // just set all species visible in this case
    displayOptions.showSpecies.resize(ns, true);
  }
  updateSpeciesToDraw();

  images.push_back(sim->getConcImage(0));
  ui->hslideTime->setEnabled(true);
  ui->hslideTime->setValue(0);
  lblGeometry->setImage(images.back());
}

void TabSimulate::stopSimulation() {
  SPDLOG_INFO("Simulation cancelled by user");
  isSimulationRunning = false;
}

void TabSimulate::useDune(bool enable) {
  if (enable) {
    simType = simulate::SimulatorType::DUNE;
  } else {
    simType = simulate::SimulatorType::Pixel;
  }
  loadModelData();
}

void TabSimulate::reset() {
  plt->clear();
  ui->hslideTime->setMinimum(0);
  ui->hslideTime->setMaximum(0);
  images.clear();
  time.clear();
  // Note: this reset is required to delete all current DUNE objects *before*
  // creating a new one, otherwise the new ones make use of the existing ones,
  // and once they are deleted it dereferences a nullptr and segfaults...
  sim.reset();
}

simulate::Options TabSimulate::getOptions() const { return simOptions; }

void TabSimulate::setOptions(const simulate::Options &options) {
  simOptions = options;
  loadModelData();
}

void TabSimulate::btnSimulate_clicked() {
  // integration time parameters
  double dtImage = ui->txtSimInterval->text().toDouble();
  int n_images =
      static_cast<int>(ui->txtSimLength->text().toDouble() / dtImage);

  QElapsedTimer simulationRuntimeTimer;
  simulationRuntimeTimer.start();
  isSimulationRunning = true;
  this->setCursor(Qt::WaitCursor);
  QApplication::processEvents();
  QTimer plotRefreshTimer;
  plotRefreshTimer.setTimerType(Qt::TimerType::VeryCoarseTimer);
  constexpr int plotMsRefreshInterval = 1000;
  plotRefreshTimer.setInterval(plotMsRefreshInterval);
  bool plotDueForRefresh = true;
  connect(&plotRefreshTimer, &QTimer::timeout, this,
          [&plotDueForRefresh]() { plotDueForRefresh = true; });
  plotRefreshTimer.start();
  // integrate Model
  for (int i_image = 0; i_image < n_images; ++i_image) {
    sim->doTimestep(dtImage);
    if (!sim->errorMessage().empty()) {
      isSimulationRunning = false;
      QMessageBox::warning(
          this, "Simulation Failed",
          QString("Simulation failed - changing the Simulation options in the "
                  "\"Advanced\" menu might help.\n\nError message: %1")
              .arg(sim->errorMessage().c_str()));
    }
    QApplication::processEvents();
    if (!isSimulationRunning) {
      break;
    }
    images.push_back(sim->getConcImage(sim->getTimePoints().size() - 1,
                                       compartmentSpeciesToDraw));
    time.push_back(time.back() + dtImage);
    int speciesIndex = 0;
    for (std::size_t ic = 0; ic < sim->getCompartmentIds().size(); ++ic) {
      for (std::size_t is = 0; is < sim->getSpeciesIds(ic).size(); ++is) {
        auto conc = sim->getAvgMinMax(static_cast<std::size_t>(time.size() - 1),
                                      ic, is);
        plt->addAvMinMaxPoint(speciesIndex, time.back(), conc);
        ++speciesIndex;
      }
    }
    lblGeometry->setImage(images.back());
    // rescale & replot plot
    if (plotDueForRefresh) {
      plt->plot->rescaleAxes(true);
      plt->plot->replot(QCustomPlot::RefreshPriority::rpQueuedReplot);
      plotDueForRefresh = false;
    }
  }
  plotRefreshTimer.stop();

  plt->setVerticalLine(time.back());

  updatePlotAndImages();

  // enable slider to choose time to display
  ui->hslideTime->setEnabled(true);
  ui->hslideTime->setMinimum(0);
  ui->hslideTime->setMaximum(time.size() - 1);
  ui->hslideTime->setValue(time.size() - 1);

  SPDLOG_INFO("simulation run-time: {}s",
              static_cast<double>(simulationRuntimeTimer.elapsed()) / 1000.0);
  this->setCursor(Qt::ArrowCursor);
}

void TabSimulate::btnSliceImage_clicked() {
  DialogImageSlice dialog(sbmlDoc.getGeometry().getImage(), images, time);
  if (dialog.exec() == QDialog::Accepted) {
    SPDLOG_DEBUG("todo: save current slice settings");
  }
}

void TabSimulate::btnExport_clicked() {
  DialogExport dialog(images, plt.get(), ui->hslideTime->value());
  if (dialog.exec() == QDialog::Accepted) {
    SPDLOG_DEBUG("todo: save current export settings");
  }
}

void TabSimulate::updateSpeciesToDraw() {
  compartmentSpeciesToDraw.clear();
  std::size_t speciesIndex = 0;
  for (const auto &compSpecies : speciesNames) {
    auto &speciesToDraw = compartmentSpeciesToDraw.emplace_back();
    for (std::size_t i = 0; i < static_cast<std::size_t>(compSpecies.size());
         ++i) {
      if (displayOptions.showSpecies[speciesIndex]) {
        speciesToDraw.push_back(i);
      }
      ++speciesIndex;
    }
  }
}

void TabSimulate::updatePlotAndImages() {

  plt->clearObservableLines();
  std::size_t colorIndex{displayOptions.showSpecies.size()};
  for (const auto &obs : observables) {
    plt->addObservableLine(obs, utils::indexedColours()[colorIndex]);
    ++colorIndex;
  }
  plt->update(displayOptions.showSpecies, displayOptions.showMinMax);
  updateSpeciesToDraw();
  // update images
  for (int iTime = 0; iTime < time.size(); ++iTime) {
    images[iTime] = sim->getConcImage(static_cast<std::size_t>(iTime),
                                      compartmentSpeciesToDraw,
                                      displayOptions.normaliseOverAllTimepoints,
                                      displayOptions.normaliseOverAllSpecies);
  }
}

void TabSimulate::btnDisplayOptions_clicked() {
  DialogDisplayOptions dialog(compartmentNames, speciesNames, displayOptions,
                              observables);
  if (dialog.exec() == QDialog::Accepted) {
    displayOptions.showMinMax = dialog.getShowMinMax();
    displayOptions.showSpecies = dialog.getShowSpecies();
    displayOptions.normaliseOverAllTimepoints =
        dialog.getNormaliseOverAllTimepoints();
    displayOptions.normaliseOverAllSpecies =
        dialog.getNormaliseOverAllSpecies();
    observables = dialog.getObservables();
    sbmlDoc.setDisplayOptions(displayOptions);
    updatePlotAndImages();
    hslideTime_valueChanged(ui->hslideTime->value());
  }
}

void TabSimulate::graphClicked(const QMouseEvent *event) {
  if (plt->plot->graphCount() == 0) {
    return;
  }
  double t = plt->xValue(event);
  int maxTimeIndex = ui->hslideTime->maximum();
  auto nearestTimeIndex =
      static_cast<int>(0.5 + maxTimeIndex * t / time.back());
  ui->hslideTime->setValue(std::clamp(nearestTimeIndex, 0, maxTimeIndex));
}

void TabSimulate::hslideTime_valueChanged(int value) {
  if (images.size() <= value) {
    return;
  }
  lblGeometry->setImage(images[value]);
  plt->setVerticalLine(time[value]);
  plt->plot->replot();
  ui->lblCurrentTime->setText(
      QString("%1%2").arg(time[value]).arg(sbmlDoc.getUnits().getTime().name));
}
