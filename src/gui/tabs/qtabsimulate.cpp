#include "qtabsimulate.hpp"

#include <qcustomplot.h>

#include <QElapsedTimer>
#include <algorithm>

//#include "dune.hpp"
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
    simulate::SimulatorType simulator = simulate::SimulatorType::DUNE;
    if (!useDuneSimulator) {
      simulator = simulate::SimulatorType::Pixel;
    }
    simulate::Simulation sim(sbmlDoc, simulator);
    lblGeometry->setImage(sim.getConcImage(0));
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
  loadModelData();
}

void QTabSimulate::btnSimulate_clicked() {
  simulate::SimulatorType simulator = simulate::SimulatorType::DUNE;
  if (!useDuneSimulator) {
    simulator = simulate::SimulatorType::Pixel;
  }
  simulate::Simulation sim(sbmlDoc, simulator);

  // integration time parameters
  double dt = ui->txtSimDt->text().toDouble();
  double dtImage = ui->txtSimInterval->text().toDouble();
  int n_images =
      static_cast<int>(ui->txtSimLength->text().toDouble() / dtImage);

  // setup plot
  time.clear();
  pltPlot->clearGraphs();
  int nTotalSpecies = 0;
  // average values
  for (std::size_t ic = 0; ic < sim.getCompartmentIds().size(); ++ic) {
    for (std::size_t is = 0; is < sim.getSpeciesIds(ic).size(); ++is) {
      auto *graph = pltPlot->addGraph();
      graph->setPen(sim.getSpeciesColors(ic)[is]);
      graph->setName(
          QString("%1 (average)")
              .arg(sbmlDoc.getSpeciesName(sim.getSpeciesIds(ic)[is].c_str())));
      graph->setScatterStyle(
          QCPScatterStyle(QCPScatterStyle::ScatterShape::ssDisc));
      ++nTotalSpecies;
    }
  }
  // min values
  for (std::size_t ic = 0; ic < sim.getCompartmentIds().size(); ++ic) {
    for (std::size_t is = 0; is < sim.getSpeciesIds(ic).size(); ++is) {
      auto *graph = pltPlot->addGraph();
      QColor c = sim.getSpeciesColors(ic)[is];
      c.setAlpha(30);
      graph->setPen(c);
      graph->setBrush(QBrush(c));
      graph->setName(
          QString("%1 (min/max range)")
              .arg(sbmlDoc.getSpeciesName(sim.getSpeciesIds(ic)[is].c_str())));
    }
  }
  // max values
  for (std::size_t ic = 0; ic < sim.getCompartmentIds().size(); ++ic) {
    for (std::size_t is = 0; is < sim.getSpeciesIds(ic).size(); ++is) {
      auto *graph = pltPlot->addGraph();
      QColor c = sim.getSpeciesColors(ic)[is];
      c.setAlpha(30);
      graph->setPen(c);
      pltPlot->graph(nTotalSpecies + static_cast<int>(is))
          ->setChannelFillGraph(graph);
      graph->setName(QString("max%1").arg(
          sbmlDoc.getSpeciesName(sim.getSpeciesIds(ic)[is].c_str())));
      pltPlot->legend->removeItem(pltPlot->legend->itemCount() - 1);
    }
  }
  pltPlot->xAxis->setLabel("time");
  pltPlot->yAxis->setLabel("concentration");
  pltPlot->xAxis->setRange(0, ui->txtSimLength->text().toDouble());

  double ymax = 0;
  time.push_back(0);
  // get initial concentrations
  int speciesIndex = 0;
  for (std::size_t ic = 0; ic < sim.getCompartmentIds().size(); ++ic) {
    for (std::size_t is = 0; is < sim.getSpeciesIds(ic).size(); ++is) {
      auto conc = sim.getAvgMinMax(0, ic, is);
      pltPlot->graph(speciesIndex)->setData({0}, {conc.avg}, true);
      pltPlot->graph(speciesIndex + nTotalSpecies)
          ->setData({0}, {conc.min}, true);
      pltPlot->graph(speciesIndex + 2 * nTotalSpecies)
          ->setData({0}, {conc.max}, true);
      ymax = std::max(ymax, conc.max);
      ++speciesIndex;
    }
  }
  images.clear();
  images.push_back(sim.getConcImage(0));
  lblGeometry->setImage(images.back());
  // ui->statusBar->showMessage("Simulating...     (press ctrl+c to cancel)");

  QElapsedTimer qElapsedTimer;
  qElapsedTimer.start();
  isSimulationRunning = true;
  this->setCursor(Qt::WaitCursor);
  QApplication::processEvents();
  // integrate Model
  for (int i_image = 0; i_image < n_images; ++i_image) {
    sim.doTimestep(dtImage, dt);
    QApplication::processEvents();
    if (!isSimulationRunning) {
      break;
    }
    images.push_back(sim.getConcImage(static_cast<std::size_t>(time.size())));
    time.push_back(time.back() + dtImage);
    speciesIndex = 0;
    for (std::size_t ic = 0; ic < sim.getCompartmentIds().size(); ++ic) {
      for (std::size_t is = 0; is < sim.getSpeciesIds(ic).size(); ++is) {
        auto conc =
            sim.getAvgMinMax(static_cast<std::size_t>(time.size() - 1), ic, is);
        pltPlot->graph(speciesIndex)->addData({time.back()}, {conc.avg}, true);
        pltPlot->graph(speciesIndex + nTotalSpecies)
            ->addData({time.back()}, {conc.min}, true);
        pltPlot->graph(speciesIndex + 2 * nTotalSpecies)
            ->addData({time.back()}, {conc.max}, true);
        ymax = std::max(ymax, conc.max);
        ++speciesIndex;
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
