#include "qtabsimulate.hpp"

#include <qcustomplot.h>

#include <QElapsedTimer>
#include <algorithm>

#include "dune.hpp"
#include "logger.hpp"
#include "qlabelmousetracker.hpp"
#include "sbml.hpp"
#include "simulate.hpp"
#include "ui_qtabsimulate.h"

QTabSimulate::QTabSimulate(sbml::SbmlDocWrapper &doc,
                           QLabelMouseTracker *mouseTracker, QWidget *parent)
    : QWidget(parent),
      ui{std::make_unique<Ui::QTabSimulate>()},
      sbmlDoc(doc),
      lblGeometry(mouseTracker) {
  ui->setupUi(this);
  pltPlot = new QCustomPlot(this);
  pltPlot->setInteraction(QCP::iRangeDrag, true);
  pltPlot->setInteraction(QCP::iRangeZoom, true);
  pltPlot->setInteraction(QCP::iSelectPlottables, false);
  pltPlot->legend->setVisible(true);
  pltTimeLine = new QCPItemStraightLine(pltPlot);
  pltTimeLine->setVisible(false);
  pltPlot->setObjectName(QString::fromUtf8("pltPlot"));
  ui->gridSimulate->addWidget(pltPlot, 1, 0, 1, 9);

  connect(ui->btnSimulate, &QPushButton::clicked, this,
          &QTabSimulate::btnSimulate_clicked);
  connect(ui->btnResetSimulation, &QPushButton::clicked, this,
          &QTabSimulate::reset);
  connect(pltPlot, &QCustomPlot::mousePress, this, &QTabSimulate::graphClicked);
  connect(ui->hslideTime, &QSlider::valueChanged, this,
          &QTabSimulate::hslideTime_valueChanged);
  connect(ui->btnStopSimulation, &QPushButton::clicked, this,
          &QTabSimulate::stopSimulation);

  ui->hslideTime->setEnabled(false);
}

QTabSimulate::~QTabSimulate() = default;

void QTabSimulate::loadModelData() {
  // ui->lblGeometryStatus->setText("Simulation concentration:");
  if (!(sbmlDoc.isValid && sbmlDoc.hasValidGeometry)) {
    return;
  }
  if (images.empty()) {
    simulate::Simulate sim(&sbmlDoc);
    for (const auto &compartmentID : sbmlDoc.compartments) {
      sim.addCompartment(&sbmlDoc.mapCompIdToGeometry.at(compartmentID));
    }
    lblGeometry->setImage(sim.getConcentrationImage());
  } else {
    ui->hslideTime->setEnabled(true);
    ui->hslideTime->setValue(0);
    hslideTime_valueChanged(0);
  }
}

void QTabSimulate::stopSimulation() {
  SPDLOG_INFO("Simulation cancelled by user");
  isSimulationRunning = false;
}

void QTabSimulate::useDune(bool enable) { useDuneSimulator = enable; }

void QTabSimulate::reset() {
  pltPlot->clearGraphs();
  pltPlot->replot();
  ui->hslideTime->setMinimum(0);
  ui->hslideTime->setMaximum(0);
  ui->hslideTime->setEnabled(false);
  images.clear();
  time.clear();
  // reset all fields to their initial values
  for (auto &field : sbmlDoc.mapSpeciesIdToField) {
    field.second.conc = field.second.init;
  }
  loadModelData();
}

void QTabSimulate::btnSimulate_clicked() {
  simulate::Simulate simPixel(&sbmlDoc);
  // add compartments
  for (const auto &compartmentID : sbmlDoc.compartments) {
    simPixel.addCompartment(&sbmlDoc.mapCompIdToGeometry.at(compartmentID));
  }
  // add membranes
  for (auto &membrane : sbmlDoc.membraneVec) {
    if (sbmlDoc.reactions.find(membrane.membraneID.c_str()) !=
        sbmlDoc.reactions.cend()) {
      simPixel.addMembrane(&membrane);
    }
  }
  std::unique_ptr<dune::DuneSimulation> simDune;
  if (useDuneSimulator) {
    simDune = std::make_unique<dune::DuneSimulation>(
        sbmlDoc, ui->txtSimDt->text().toDouble(), lblGeometry->size());
  }

  // integration time parameters
  double dt = ui->txtSimDt->text().toDouble();
  double dtImage = ui->txtSimInterval->text().toDouble();
  int n_images =
      static_cast<int>(ui->txtSimLength->text().toDouble() / dtImage);
  int n_steps = static_cast<int>(dtImage / dt);

  // setup plot
  pltPlot->clearGraphs();
  // average values
  for (std::size_t i = 0; i < simPixel.speciesID.size(); ++i) {
    auto *graph = pltPlot->addGraph();
    graph->setPen(simPixel.field[i]->colour);
    graph->setName(
        QString("%1 (average)")
            .arg(sbmlDoc.getSpeciesName(simPixel.speciesID[i].c_str())));
    graph->setScatterStyle(
        QCPScatterStyle(QCPScatterStyle::ScatterShape::ssDisc));
    if (!useDuneSimulator) {
      graph->setScatterSkip(n_steps - 1);
    }
  }
  // min values
  for (std::size_t i = 0; i < simPixel.speciesID.size(); ++i) {
    auto *graph = pltPlot->addGraph();
    QColor c = simPixel.field[i]->colour;
    c.setAlpha(30);
    graph->setPen(c);
    graph->setBrush(QBrush(c));
    graph->setName(
        QString("%1 (min/max range)")
            .arg(sbmlDoc.getSpeciesName(simPixel.speciesID[i].c_str())));
  }
  // max values
  for (std::size_t i = 0; i < simPixel.speciesID.size(); ++i) {
    auto *graph = pltPlot->addGraph();
    QColor c = simPixel.field[i]->colour;
    c.setAlpha(30);
    graph->setPen(c);
    pltPlot->graph(static_cast<int>(simPixel.speciesID.size() + i))
        ->setChannelFillGraph(graph);
    graph->setName(QString("max%1").arg(
        sbmlDoc.getSpeciesName(simPixel.speciesID[i].c_str())));
    pltPlot->legend->removeItem(pltPlot->legend->itemCount() - 1);
  }
  pltPlot->xAxis->setLabel("time");
  pltPlot->yAxis->setLabel("concentration");
  pltPlot->xAxis->setRange(0, ui->txtSimLength->text().toDouble());

  double ymax = 0;
  time.push_back(0);
  // get initial concentrations
  for (std::size_t s = 0; s < simPixel.field.size(); ++s) {
    QVector<double> c_{0};
    QVector<double> min_{0};
    QVector<double> max_{0};
    if (useDuneSimulator) {
      c_[0] = simDune->getAverageConcentration(simPixel.field[s]->speciesID);
      min_[0] = simDune->getMinConcentration(simPixel.field[s]->speciesID);
      max_[0] = simDune->getMaxConcentration(simPixel.field[s]->speciesID);
    } else {
      c_[0] = simPixel.field[s]->getMeanConcentration();
      min_[0] = simPixel.field[s]->getMinConcentration();
      max_[0] = simPixel.field[s]->getMaxConcentration();
    }
    int speciesIndex = static_cast<int>(s);
    int nSpecies = static_cast<int>(simPixel.field.size());
    pltPlot->graph(speciesIndex)->setData(time, c_, true);
    pltPlot->graph(speciesIndex + nSpecies)->setData(time, min_, true);
    pltPlot->graph(speciesIndex + 2 * nSpecies)->setData(time, max_, true);
    ymax = std::max(ymax, max_[0]);
  }
  images.clear();
  if (useDuneSimulator) {
    images.push_back(simDune->getConcImage());
  } else {
    images.push_back(simPixel.getConcentrationImage());
  }
  lblGeometry->setImage(images.back());
  // ui->statusBar->showMessage("Simulating...     (press ctrl+c to cancel)");

  QElapsedTimer qElapsedTimer;
  qElapsedTimer.start();
  isSimulationRunning = true;
  this->setCursor(Qt::WaitCursor);
  QApplication::processEvents();
  // integrate Model
  QVector<double> subTime(n_steps, 0);
  std::vector<QVector<double>> subConcAv(simPixel.field.size());
  std::vector<QVector<double>> subConcMin(simPixel.field.size());
  std::vector<QVector<double>> subConcMax(simPixel.field.size());
  for (std::size_t s = 0; s < simPixel.field.size(); ++s) {
    subConcAv[s].resize(n_steps);
    subConcMin[s].resize(n_steps);
    subConcMax[s].resize(n_steps);
  }
  for (int i_image = 0; i_image < n_images; ++i_image) {
    if (useDuneSimulator) {
      simDune->doTimestep(ui->txtSimInterval->text().toDouble());
    } else {
      for (int i_step = 0; i_step < n_steps; ++i_step) {
        subTime[i_step] = time.back() + dt + dt * i_step;
        simPixel.integrateForwardsEuler(dt);
        for (std::size_t s = 0; s < simPixel.field.size(); ++s) {
          subConcAv[s][i_step] = simPixel.field[s]->getMeanConcentration();
          subConcMin[s][i_step] = simPixel.field[s]->getMinConcentration();
          subConcMax[s][i_step] = simPixel.field[s]->getMaxConcentration();
        }
      }
    }
    QApplication::processEvents();
    if (!isSimulationRunning) {
      break;
    }
    if (useDuneSimulator) {
      images.push_back(simDune->getConcImage());
    } else {
      images.push_back(simPixel.getConcentrationImage());
    }
    time.push_back(time.back() + dtImage);
    for (std::size_t s = 0; s < simPixel.field.size(); ++s) {
      int speciesIndex = static_cast<int>(s);
      int nSpecies = static_cast<int>(simPixel.field.size());
      if (useDuneSimulator) {
        QVector<double> c_{
            simDune->getAverageConcentration(simPixel.field[s]->speciesID)};
        QVector<double> min_{
            simDune->getMinConcentration(simPixel.field[s]->speciesID)};
        QVector<double> max_{
            simDune->getMaxConcentration(simPixel.field[s]->speciesID)};
        ymax = std::max(ymax, max_[0]);
        pltPlot->graph(speciesIndex)->addData({time.back()}, c_, true);
        pltPlot->graph(speciesIndex + nSpecies)
            ->addData({time.back()}, min_, true);
        pltPlot->graph(speciesIndex + 2 * nSpecies)
            ->addData({time.back()}, max_, true);
      } else {
        pltPlot->graph(speciesIndex)->addData(subTime, subConcAv[s], true);
        pltPlot->graph(speciesIndex + nSpecies)
            ->addData(subTime, subConcMin[s], true);
        pltPlot->graph(speciesIndex + 2 * nSpecies)
            ->addData(subTime, subConcMax[s], true);
        ymax = std::max(ymax, *std::max_element(subConcMax[s].cbegin(),
                                                subConcMax[s].cend()));
      }
    }
    lblGeometry->setImage(images.back());
    // rescale & replot plot
    pltPlot->yAxis->setRange(0, 1.2 * ymax);
    pltPlot->replot();
  }

  // display vertical at current time point
  pltTimeLine->setVisible(true);
  pltTimeLine->point1->setCoords(time.back(), 0);
  pltTimeLine->point2->setCoords(time.back(), 1);

  // rescale & replot plot
  pltPlot->xAxis->rescale();
  pltPlot->replot();

  // enable slider to choose time to display
  ui->hslideTime->setEnabled(true);
  ui->hslideTime->setMinimum(0);
  ui->hslideTime->setMaximum(time.size() - 1);
  ui->hslideTime->setValue(time.size() - 1);

  // ui->statusBar->showMessage("Simulation complete.");
  SPDLOG_INFO("simulation run-time: {}s",
              static_cast<double>(qElapsedTimer.elapsed()) / 1000.0);
  this->setCursor(Qt::ArrowCursor);
}

void QTabSimulate::graphClicked(const QMouseEvent *event) {
  if (pltPlot->graphCount() == 0) {
    return;
  }
  double key;
  double val;
  pltPlot->graph(0)->pixelsToCoords(static_cast<double>(event->x()),
                                    static_cast<double>(event->y()), key, val);
  int max = ui->hslideTime->maximum();
  int nearest = static_cast<int>(0.5 + max * key / time.back());
  ui->hslideTime->setValue(std::clamp(nearest, 0, max));
}

void QTabSimulate::hslideTime_valueChanged(int value) {
  if (images.size() > value) {
    lblGeometry->setImage(images[value]);
    pltTimeLine->point1->setCoords(time[value], 0);
    pltTimeLine->point2->setCoords(time[value], 1);
    if (!pltPlot->xAxis->range().contains(time[value])) {
      pltPlot->rescaleAxes();
    }
    pltPlot->replot();
  }
}
