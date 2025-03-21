#include "tabsimulate.hpp"
#include "dialogexport.hpp"
#include "dialogimage.hpp"
#include "dialogimageslice.hpp"
#include "guiutils.hpp"
#include "qlabelmousetracker.hpp"
#include "qvoxelrenderer.hpp"
#include "sme/logger.hpp"
#include "sme/mesh2d.hpp"
#include "sme/mesh3d.hpp"
#include "sme/model.hpp"
#include "sme/serialization.hpp"
#include "sme/simulate.hpp"
#include "sme/simulate_options.hpp"
#include "sme/utils.hpp"
#include "ui_tabsimulate.h"
#include <QMessageBox>
#include <QProgressDialog>
#include <algorithm>
#include <future>

TabSimulate::TabSimulate(sme::model::Model &m, QLabelMouseTracker *mouseTracker,
                         QVoxelRenderer *voxelRenderer, QWidget *parent)
    : QWidget(parent), ui{std::make_unique<Ui::TabSimulate>()}, model{m},
      lblGeometry{mouseTracker}, voxGeometry{voxelRenderer},
      plt{std::make_unique<PlotWrapper>("Average Concentration", this)} {
  ui->setupUi(this);
  ui->gridSimulate->addWidget(plt->plot, 1, 0, 1, 7);

  progressDialog =
      new QProgressDialog("Simulating model...", "Stop simulation", 0, 1, this);
  progressDialog->reset();
  connect(progressDialog, &QProgressDialog::canceled, this,
          &TabSimulate::stopSimulation);
  connect(ui->btnSimulate, &QPushButton::clicked, this,
          &TabSimulate::btnSimulate_clicked);
  connect(ui->btnResetSimulation, &QPushButton::clicked, this,
          &TabSimulate::reset);
  connect(plt->plot, &QCustomPlot::mousePress, this,
          &TabSimulate::graphClicked);
  connect(ui->hslideTime, &QSlider::valueChanged, this,
          &TabSimulate::hslideTime_valueChanged);
  connect(ui->btnSliceImage, &QPushButton::clicked, this,
          &TabSimulate::btnSliceImage_clicked);
  connect(ui->btnExport, &QPushButton::clicked, this,
          &TabSimulate::btnExport_clicked);
  connect(ui->btnDisplayOptions, &QPushButton::clicked, this,
          &TabSimulate::btnDisplayOptions_clicked);

  // timer to call updatePlotAndImages every second
  plotRefreshTimer.setTimerType(Qt::TimerType::VeryCoarseTimer);
  constexpr int plotMsRefreshInterval{1000};
  plotRefreshTimer.setInterval(plotMsRefreshInterval);
  connect(&plotRefreshTimer, &QTimer::timeout, this, [this]() {
    updatePlotAndImages();
    if (!sim->getIsRunning()) {
      // simulation finished - repeat above first in case new data was added
      updatePlotAndImages();
      finalizePlotAndImages();
    }
  });
  useSimulator("DUNE");
  ui->hslideTime->setEnabled(false);
  ui->btnResetSimulation->setEnabled(false);

  SPDLOG_CRITICAL(
      "Tabsimulate setup done: {}",
      static_cast<int>(model.getSimulationSettings().simulatorType));
}

TabSimulate::~TabSimulate() = default;

static void importModelTimesAndIntervals(
    Ui::TabSimulate *ui,
    const std::vector<std::pair<std::size_t, double>> &times) {
  QString simLength{};
  QString simInterval{};
  for (const auto &[n, t] : times) {
    simLength.append(QString::number(static_cast<double>(n) * t));
    simLength.append(";");
    simInterval.append(QString::number(t));
    simInterval.append(";");
  }
  simLength.chop(1);
  simInterval.chop(1);
  if (!simLength.isEmpty()) {
    ui->txtSimLength->setText(simLength);
  }
  if (!simInterval.isEmpty()) {
    ui->txtSimInterval->setText(simInterval);
  }
}

std::string
TabSimulate::chooseAlternativeSimulator(sme::simulate::SimulatorType simtype) {
  if (simtype == sme::simulate::SimulatorType::DUNE) {
    return "Pixel";
  } else if (simtype == sme::simulate::SimulatorType::DUNESteadyState) {
    return "PixelSteadyState";
  } else if (simtype == sme::simulate::SimulatorType::Pixel) {
    return "DUNE";
  } else if (simtype == sme::simulate::SimulatorType::PixelSteadyState) {
    return "DUNESteadyState";
  } else {
    return "";
  }
}

sme::simulate::SimulatorType
TabSimulate::chooseAlternativeSimulator(std::string_view simtype) {
  if (simtype == "DUNE") {
    return sme::simulate::SimulatorType::Pixel;
  } else if (simtype == "DUNESteadyState") {
    return sme::simulate::SimulatorType::PixelSteadyState;
  } else if (simtype == "Pixel") {
    return sme::simulate::SimulatorType::DUNE;
  } else if (simtype == "PixelSteadyState") {
    return sme::simulate::SimulatorType::DUNESteadyState;
  } else {
    return sme::simulate::SimulatorType::Pixel;
  }
}

void TabSimulate::loadModelData() {
  SPDLOG_CRITICAL(
      "load model data with solver: {}",
      static_cast<int>((model.getSimulationSettings().simulatorType)));

  if (sim != nullptr && sim->getIsRunning()) {
    return;
  }
  if (!(model.getIsValid() && model.getGeometry().getIsValid())) {
    ui->hslideTime->setEnabled(false);
    ui->btnSimulate->setEnabled(false);
    return;
  }
  if (sim != nullptr && sim->getIsRunning()) {
    // wait for any existing running simulation to stop
    sim->requestStop();
    simSteps.wait();
  }
  if (model.getSimulationSettings().simulatorType ==
          sme::simulate::SimulatorType::DUNE or
      model.getSimulationSettings().simulatorType ==
          sme::simulate::SimulatorType::DUNESteadyState) {
    SPDLOG_CRITICAL("    Some DUNE solver is selected");
    QString duneInvalidTitle{};
    QString duneInvalidMessage{};
    if (!model.getGeometry().getIsMeshValid()) {
      duneInvalidTitle = "Invalid Mesh";
      duneInvalidMessage =
          "Mesh geometry is not valid, and is required for a DuneCopasi "
          "simulation.";
    } else if (model.getSpecies().containsNonSpatialReactiveSpecies()) {
      duneInvalidTitle = "Non-spatial species not supported";
      duneInvalidMessage =
          "The model contains non-spatial species, which are not "
          "currently supported by DuneCopasi.";
    }
    if (!duneInvalidTitle.isEmpty()) {
      ui->btnSimulate->setEnabled(false);
      SPDLOG_CRITICAL(
          "   DUNE simulation setup failed, trying to select alternative: {}",
          duneInvalidMessage.toStdString());
      auto result{QMessageBox::question(
          this, duneInvalidTitle,
          duneInvalidMessage +
              " Would you like to use the Pixel simulator instead?",
          QMessageBox::Yes | QMessageBox::No)};
      if (result == QMessageBox::Yes) {
        if (model.getSimulationSettings().simulatorType ==
            sme::simulate::SimulatorType::DUNE) {
          model.getSimulationSettings().simulatorType =
              sme::simulate::SimulatorType::Pixel;
        } else {
          model.getSimulationSettings().simulatorType =
              sme::simulate::SimulatorType::PixelSteadyState;
        }
      }
      return;
    }
  }
  plt->clear();
  ui->hslideTime->setMinimum(0);
  ui->hslideTime->setMaximum(0);
  images.clear();
  time.clear();
  // Note: this reset is required to delete all current DUNE objects *before*
  // creating a new one, otherwise the new ones make use of the existing ones,
  // and once they are deleted it dereferences a nullptr and segfaults...
  sim.reset();
  sim = std::make_unique<sme::simulate::Simulation>(model);
  if (!sim->errorMessage().empty()) {
    ui->btnSimulate->setEnabled(false);
    SPDLOG_CRITICAL("something went wrong with the simulation setup, trying to "
                    "select other solver");
    QString alternativeSim{
        chooseAlternativeSimulator(model.getSimulationSettings().simulatorType)
            .c_str()};
    if (QMessageBox::question(
            this, "Simulation Setup Failed",
            QString(
                "Simulation setup failed.\n\nError message: %1\n\nWould you "
                "like to try using the %2 simulator instead?")
                .arg(sim->errorMessage().c_str())
                .arg(alternativeSim),
            QMessageBox::Yes | QMessageBox::No) == QMessageBox::Yes) {
      useSimulator(chooseAlternativeSimulator(
          model.getSimulationSettings().simulatorType));
    }
    return;
  }

  if (importTimesAndIntervals) {
    importModelTimesAndIntervals(ui.get(), model.getSimulationSettings().times);
  }
  importTimesAndIntervals = false;
  if (ui->txtSimLength->text().isEmpty()) {
    ui->txtSimLength->setText("100");
  }
  if (ui->txtSimInterval->text().isEmpty()) {
    ui->txtSimInterval->setText("1");
  }
  ui->btnSimulate->setEnabled(true);
  ui->btnResetSimulation->setEnabled(false);

  // setup species names
  speciesNames.clear();
  compartmentNames.clear();
  std::size_t nSpecies{0};
  for (std::size_t ic = 0; ic < sim->getCompartmentIds().size(); ++ic) {
    compartmentNames.push_back(
        model.getCompartments().getName(sim->getCompartmentIds()[ic].c_str()));
    auto &names = speciesNames.emplace_back();
    for (const auto &sId : sim->getSpeciesIds(ic)) {
      names.push_back(model.getSpecies().getName(sId.c_str()));
      ++nSpecies;
    }
  }
  // setup plot
  plt->plot->xAxis->setLabel(
      QString("time (%1)").arg(model.getUnits().getTime().name));
  plt->plot->yAxis->setLabel(
      QString("concentration (%1)").arg(model.getUnits().getConcentration()));
  plt->plot->xAxis->setRange(0, ui->txtSimLength->text().toDouble());
  // add lines
  for (std::size_t ic = 0; ic < sim->getCompartmentIds().size(); ++ic) {
    for (std::size_t is = 0; is < sim->getSpeciesIds(ic).size(); ++is) {
      QColor col = sim->getSpeciesColors(ic)[is];
      QString name =
          model.getSpecies().getName(sim->getSpeciesIds(ic)[is].c_str());
      plt->addAvMinMaxLine(name, col);
    }
  }
  displayOptions = model.getDisplayOptions();
  if (displayOptions.showSpecies.size() != nSpecies) {
    // show species count doesn't match actual number of species
    // user probably added/removed species to model
    // just set all species visible in this case
    displayOptions.showSpecies.resize(nSpecies, true);
  }
  updateSpeciesToDraw();
  updatePlotAndImages();
  finalizePlotAndImages();
}

void TabSimulate::stopSimulation() {
  SPDLOG_INFO("Simulation stop requested by user");
  sim->requestStop();
}

void TabSimulate::useSimulator(std::string simulator) {
  SPDLOG_CRITICAL("useSimulator: {}", simulator);
  if (simulator == "Dune") {
    SPDLOG_CRITICAL("  useSimulator selecting DUNE: {}", simulator);
    model.getSimulationSettings().simulatorType =
        sme::simulate::SimulatorType::DUNE;
  } else if (simulator == "DuneSteadyState") {
    SPDLOG_CRITICAL("  useSimulator selecting DUNESteadyState: {}", simulator);
    model.getSimulationSettings().simulatorType =
        sme::simulate::SimulatorType::DUNESteadyState;
  } else if (simulator == "PixelSteadyState") {
    SPDLOG_CRITICAL("  useSimulator selecting PixelSteadyState: {}", simulator);
    model.getSimulationSettings().simulatorType =
        sme::simulate::SimulatorType::PixelSteadyState;
  } else {
    SPDLOG_CRITICAL("  useSimulator selecting  Pixel: {}", simulator);
    model.getSimulationSettings().simulatorType =
        sme::simulate::SimulatorType::Pixel;
  }
  loadModelData();
}

void TabSimulate::importTimesAndIntervalsOnNextLoad() {
  importTimesAndIntervals = true;
}

void TabSimulate::reset() {
  if (sim != nullptr && sim->getIsRunning()) {
    // stop any existing running simulation
    sim->requestStop();
    simSteps.wait();
  }
  model.getSimulationData().clear();
  importModelTimesAndIntervals(ui.get(), model.getSimulationSettings().times);
  model.getSimulationSettings().times.clear();
  importTimesAndIntervals = false;
  loadModelData();
}

void TabSimulate::setOptions(const sme::simulate::Options &options) {
  model.getSimulationSettings().options = options;
  loadModelData();
}

void TabSimulate::invertYAxis(bool enable) { flipYAxis = enable; }

void TabSimulate::btnSimulate_clicked() {
  SPDLOG_CRITICAL("btnSimulate_clicked");
  auto simulationTimes{sme::simulate::parseSimulationTimes(
      ui->txtSimLength->text(), ui->txtSimInterval->text())};

  for (auto [n, dt] : simulationTimes.value()) {
    SPDLOG_CRITICAL("   times: {} x {}", n, dt);
  }

  if (!simulationTimes.has_value()) {
    QMessageBox::warning(this, "Invalid simulation length or image interval",
                         "Invalid simulation length or image interval");
    return;
  }
  QString imageIntervalsText;
  for (const auto &[n, dt] : simulationTimes.value()) {
    SPDLOG_DEBUG("{} x {}", n, dt);
    imageIntervalsText.append(QString::number(dt));
    imageIntervalsText.append(";");
  }
  imageIntervalsText.chop(1);
  ui->txtSimInterval->setText(imageIntervalsText);

  // display modal progress dialog box
  progressDialog->setWindowModality(Qt::WindowModal);
  progressDialog->setValue(static_cast<int>(time.size()) - 1);
  progressDialog->show();
  ui->btnSimulate->setEnabled(false);
  ui->btnResetSimulation->setEnabled(false);
  auto progressMax{static_cast<int>(time.size())};
  for (const auto &simulationTime : simulationTimes.value()) {
    progressMax += static_cast<int>(simulationTime.first);
  }
  progressDialog->setMaximum(progressMax);

  this->setCursor(Qt::WaitCursor);
  // start simulation in a new thread
  simSteps = std::async(
      std::launch::async, &sme::simulate::Simulation::doMultipleTimesteps,
      sim.get(), simulationTimes.value(), -1.0, std::function<bool()>{});
  // start timer to periodically update simulation results
  plotRefreshTimer.start();
  SPDLOG_CRITICAL("btnSimulate_clicked finished");
}

void TabSimulate::btnSliceImage_clicked() {
  DialogImageSlice dialog(model.getGeometry().getImages(), images, time,
                          flipYAxis);
  if (dialog.exec() == QDialog::Accepted) {
    SPDLOG_DEBUG("todo: save current slice settings");
  }
}

void TabSimulate::btnExport_clicked() {
  DialogExport dialog(images, plt.get(), model, *sim.get(),
                      ui->hslideTime->value());
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
  if (sim == nullptr) {
    return;
  }
  std::size_t n0{static_cast<std::size_t>(time.size())};
  std::size_t n{sim->getNCompletedTimesteps()};
  if (!sim->getIsStopping()) {
    progressDialog->setValue(static_cast<int>(n0));
  }
  for (std::size_t i = n0; i < n; ++i) {
    SPDLOG_DEBUG("adding timepoint {}", i);
    // process new results
    images.push_back(sim->getConcImage(i, compartmentSpeciesToDraw));
    time.push_back(sim->getTimePoints()[i]);
    int speciesIndex = 0;
    for (std::size_t ic = 0; ic < sim->getCompartmentIds().size(); ++ic) {
      for (std::size_t is = 0; is < sim->getSpeciesIds(ic).size(); ++is) {
        auto avgMinMax = sim->getAvgMinMax(i, ic, is);
        plt->addAvMinMaxPoint(speciesIndex, time.back(), avgMinMax);
        ++speciesIndex;
      }
    }
    lblGeometry->setImage(images.back());
    voxGeometry->setImage(images.back());
    plt->plot->rescaleAxes(true);
    plt->plot->replot(QCustomPlot::RefreshPriority::rpQueuedReplot);
  }
}

void TabSimulate::finalizePlotAndImages() {
  SPDLOG_DEBUG("simulation finished");
  // simulation finished..
  progressDialog->reset();
  plotRefreshTimer.stop();
  this->setCursor(Qt::ArrowCursor);
  ui->btnSimulate->setEnabled(true);
  ui->btnResetSimulation->setEnabled(true);
  // ..but failed
  if (const auto &err{sim->errorMessage()};
      !err.empty() && err != "Simulation stopped early") {
    DialogImage(
        this, "Simulation Failed",
        QString("Simulation failed - changing the Simulation options in the "
                "\"Advanced\" menu might help.\n\nError message: %1")
            .arg(err.c_str()),
        sim->errorImages())
        .exec();
    return;
  }
  // .. and succeeded
  // add custom observables to plot
  plt->clearObservableLines();
  std::size_t colorIndex{displayOptions.showSpecies.size()};
  for (const auto &obs : observables) {
    plt->addObservableLine(obs, sme::common::indexedColors()[colorIndex]);
    ++colorIndex;
  }
  plt->update(displayOptions.showSpecies, displayOptions.showMinMax);
  updateSpeciesToDraw();
  // update all images
  for (int iTime = 0; iTime < time.size(); ++iTime) {
    images[iTime] = sim->getConcImage(static_cast<std::size_t>(iTime),
                                      compartmentSpeciesToDraw,
                                      displayOptions.normaliseOverAllTimepoints,
                                      displayOptions.normaliseOverAllSpecies);
  }
  plt->setVerticalLine(time.back());
  // enable slider to choose time to display
  ui->hslideTime->setEnabled(true);
  ui->hslideTime->setMinimum(0);
  int sliderValue{static_cast<int>(time.size()) - 1};
  ui->hslideTime->setMaximum(sliderValue);
  ui->hslideTime->setValue(sliderValue);
  if (const auto &err{sim->errorMessage()}; err == "Simulation stopped early") {
    // reset simulation after early stop as it may contain a partial timestep
    SPDLOG_INFO("resetting simulation after early stop");
    sim.reset();
    sim = std::make_unique<sme::simulate::Simulation>(model);
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
    model.setDisplayOptions(displayOptions);
    updatePlotAndImages();
    finalizePlotAndImages();
    hslideTime_valueChanged(ui->hslideTime->value());
  }
}

void TabSimulate::graphClicked(const QMouseEvent *event) {
  if (plt->plot->graphCount() == 0) {
    return;
  }
  double t{plt->xValue(event)};
  const auto maxIndex{ui->hslideTime->maximum()};
  // index of first element of time >= t
  auto timeIndex{static_cast<int>(std::distance(
      time.begin(), std::lower_bound(time.begin(), time.end(), t)))};
  if (timeIndex > maxIndex) {
    timeIndex = maxIndex;
  } else if (timeIndex > 0) {
    // check if previous element is closer, if so use that one instead
    auto delta{std::abs(time[timeIndex] - t)};
    auto deltaPrev{std::abs(time[timeIndex - 1] - t)};
    if (deltaPrev < delta) {
      --timeIndex;
    }
  }
  ui->hslideTime->setValue(timeIndex);
}

void TabSimulate::hslideTime_valueChanged(int value) {
  if (images.size() <= value) {
    return;
  }
  lblGeometry->setImage(images[value]);
  voxGeometry->setImage(images[value]);
  plt->setVerticalLine(time[value]);
  plt->plot->replot();
  ui->lblCurrentTime->setText(
      QString("%1%2").arg(time[value]).arg(model.getUnits().getTime().name));
}
