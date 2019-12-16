#include "qtabsimulate.hpp"

#include <qcustomplot.h>

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
  pltPlot->setObjectName(QString::fromUtf8("pltPlot"));
  ui->gridSimulate->addWidget(pltPlot, 1, 0, 1, 8);

  connect(ui->btnSimulate, &QPushButton::clicked, this,
          &QTabSimulate::btnSimulate_clicked);
  connect(ui->btnResetSimulation, &QPushButton::clicked, this,
          &QTabSimulate::reset);
  connect(pltPlot, &QCustomPlot::plottableClick, this,
          &QTabSimulate::graphClicked);
  connect(ui->hslideTime, &QSlider::valueChanged, this,
          &QTabSimulate::hslideTime_valueChanged);

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

void QTabSimulate::stopSimulation() { isSimulationRunning = false; }

void QTabSimulate::useDune(bool enable) { useDuneSimulator = enable; }

void QTabSimulate::reset() {
  pltPlot->clearGraphs();
  pltPlot->replot();
  ui->hslideTime->setMinimum(0);
  ui->hslideTime->setMaximum(0);
  ui->hslideTime->setEnabled(false);
  images.clear();
  // reset all fields to their initial values
  for (auto &field : sbmlDoc.mapSpeciesIdToField) {
    field.second.conc = field.second.init;
  }
  loadModelData();
}

void QTabSimulate::btnSimulate_clicked() {
  // simple 2d spatial simulation
  simulate::Simulate sim(&sbmlDoc);
  // add compartments
  for (const auto &compartmentID : sbmlDoc.compartments) {
    sim.addCompartment(&sbmlDoc.mapCompIdToGeometry.at(compartmentID));
  }
  // add membranes
  for (auto &membrane : sbmlDoc.membraneVec) {
    if (sbmlDoc.reactions.find(membrane.membraneID.c_str()) !=
        sbmlDoc.reactions.cend()) {
      sim.addMembrane(&membrane);
    }
  }

  // Dune simulation
  dune::DuneSimulation duneSim(sbmlDoc, ui->txtSimDt->text().toDouble(),
                               lblGeometry->size());

  // get initial concentrations
  QVector<double> time{0};
  std::vector<QVector<double>> conc(sim.field.size());
  for (std::size_t s = 0; s < sim.field.size(); ++s) {
    if (useDuneSimulator) {
      conc[s].push_back(
          duneSim.getAverageConcentration(sim.field[s]->speciesID));
    } else {
      conc[s].push_back(sim.field[s]->getMeanConcentration());
    }
  }
  images.clear();
  if (useDuneSimulator) {
    images.push_back(duneSim.getConcImage());
  } else {
    images.push_back(sim.getConcentrationImage());
  }
  lblGeometry->setImage(images.back());
  // ui->statusBar->showMessage("Simulating...     (press ctrl+c to cancel)");

  QTime qtime;
  qtime.start();
  isSimulationRunning = true;
  this->setCursor(Qt::WaitCursor);
  QApplication::processEvents();
  // integrate Model
  double t = 0;
  double dt = ui->txtSimDt->text().toDouble();
  int n_images = static_cast<int>(ui->txtSimLength->text().toDouble() /
                                  ui->txtSimInterval->text().toDouble());
  int n_steps = static_cast<int>(ui->txtSimInterval->text().toDouble() / dt);
  for (int i_image = 0; i_image < n_images; ++i_image) {
    if (useDuneSimulator) {
      duneSim.doTimestep(ui->txtSimInterval->text().toDouble());
      t += ui->txtSimInterval->text().toDouble();
    } else {
      for (int i_step = 0; i_step < n_steps; ++i_step) {
        t += dt;
        sim.integrateForwardsEuler(dt);
        QApplication::processEvents();
        if (!isSimulationRunning) {
          break;
        }
      }
    }
    QApplication::processEvents();
    if (!isSimulationRunning) {
      break;
    }
    if (useDuneSimulator) {
      images.push_back(duneSim.getConcImage());
    } else {
      images.push_back(sim.getConcentrationImage());
    }
    for (std::size_t s = 0; s < sim.field.size(); ++s) {
      if (useDuneSimulator) {
        conc[s].push_back(
            duneSim.getAverageConcentration(sim.field[s]->speciesID));
      } else {
        conc[s].push_back(sim.field[s]->getMeanConcentration());
      }
    }
    time.push_back(t);
    lblGeometry->setImage(images.back());
    // ui->statusBar->showMessage(
    //    QString("Simulating... %1% (press ctrl+c to cancel)")
    //        .arg(QString::number(static_cast<int>(
    //            100 * t / ui->txtSimLength->text().toDouble()))));
  }

  // plot results
  pltPlot->clearGraphs();
  pltPlot->setInteraction(QCP::iRangeDrag, true);
  pltPlot->setInteraction(QCP::iRangeZoom, true);
  pltPlot->setInteraction(QCP::iSelectPlottables, true);
  pltPlot->legend->setVisible(true);
  for (std::size_t i = 0; i < sim.speciesID.size(); ++i) {
    auto *graph = pltPlot->addGraph();
    graph->setData(time, conc[i]);
    graph->setPen(sim.field[i]->colour);
    graph->setName(sbmlDoc.getSpeciesName(sim.speciesID[i].c_str()));
  }
  pltPlot->xAxis->setLabel("time");
  pltPlot->yAxis->setLabel("concentration");
  pltPlot->xAxis->setRange(time.front(), time.back());
  double ymax = *std::max_element(conc[0].cbegin(), conc[0].cend());
  for (std::size_t i = 1; i < conc.size(); ++i) {
    ymax = std::max(ymax, *std::max_element(conc[i].cbegin(), conc[i].cend()));
  }
  pltPlot->yAxis->setRange(0, 1.2 * ymax);
  pltPlot->replot();

  // enable slider to choose time to display
  ui->hslideTime->setEnabled(true);
  ui->hslideTime->setMinimum(0);
  ui->hslideTime->setMaximum(time.size() - 1);
  ui->hslideTime->setValue(time.size() - 1);

  // ui->statusBar->showMessage("Simulation complete.");
  SPDLOG_INFO("simulation run-time: {}", qtime.elapsed());
  this->setCursor(Qt::ArrowCursor);
}

void QTabSimulate::graphClicked(QCPAbstractPlottable *plottable,
                                int dataIndex) {
  double dataValue = plottable->interface1D()->dataMainValue(dataIndex);
  QString message =
      QString("Clicked on graph '%1' at data point #%2 with value %3.")
          .arg(plottable->name())
          .arg(dataIndex)
          .arg(dataValue);
  qDebug() << message;
  ui->hslideTime->setValue(dataIndex);
}

void QTabSimulate::hslideTime_valueChanged(int value) {
  if (images.size() > value) {
    lblGeometry->setImage(images[value]);
  }
}
