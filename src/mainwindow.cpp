#include <QErrorMessage>
#include <QFileDialog>
#include <QInputDialog>
#include <QMessageBox>
#include <QString>
#include <QStringListModel>

#include "mainwindow.h"
#include "sbml.h"
#include "simulate.h"
#include "ui_mainwindow.h"

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent), ui(new Ui::MainWindow) {
  ui->setupUi(this);

  // for debugging: import an image on startup
  ui->lblGeometry->importGeometry(
      "../spatial-model-editor/two-blobs-100x100.bmp");

  compartmentMenu = std::unique_ptr<QMenu>(new QMenu());
}

MainWindow::~MainWindow() { delete ui; }

void MainWindow::on_action_About_triggered() {
  QMessageBox msgBox;
  msgBox.setWindowTitle("About");
  QString info("Spatial Model Editor\n");
  info.append("github.com/lkeegan/spatial-model-editor\n\n");
  info.append("Included libraries:\n");
  info.append("\nQt5:\t");
  info.append(QT_VERSION_STR);
  info.append("\nlibSBML:\t");
  info.append(libsbml::getLibSBMLDottedVersion());
  for (const auto &dep : {"expat", "libxml", "xerces-c", "bzip2", "zip"}) {
    if (libsbml::isLibSBMLCompiledWith(dep) != 0) {
      info.append("\n");
      info.append(dep);
      info.append(":\t");
      info.append(libsbml::getLibSBMLDependencyVersionOf(dep));
    }
  }
  msgBox.setText(info);
  msgBox.exec();
}

void MainWindow::on_actionE_xit_triggered() { QApplication::quit(); }

void MainWindow::on_action_Open_SBML_file_triggered() {
  // load SBML file
  QString filename = QFileDialog::getOpenFileName();
  if (!filename.isEmpty()) {
    sbml_doc.loadFile(qPrintable(filename));
  }

  // update raw XML display
  ui->txtSBML->setText(sbml_doc.xml);

  // update list of reactions
  ui->listReactions->clear();
  ui->listReactions->insertItems(0, sbml_doc.reactions);

  // update list of functions
  ui->listFunctions->clear();
  ui->listFunctions->insertItems(0, sbml_doc.functions);

  // update possible compartments in compartmentMenu
  compartmentMenu->clear();
  compartmentMenu->addAction("none");
  for (auto c : sbml_doc.compartments) {
    compartmentMenu->addAction(c);
  }
  ui->btnChangeCompartment->setMenu(compartmentMenu.get());
}

void MainWindow::on_action_Save_SBML_file_triggered() {
  QMessageBox msgBox;
  msgBox.setText("todo");
  msgBox.exec();
}

void MainWindow::on_actionAbout_Qt_triggered() { QMessageBox::aboutQt(this); }

void MainWindow::on_actionGeometry_from_image_triggered() {
  QString filename = QFileDialog::getOpenFileName();
  ui->lblGeometry->importGeometry(filename);
}

void MainWindow::on_lblGeometry_mouseClicked() {
  QPalette sample_palette;
  sample_palette.setColor(QPalette::Window,
                          QColor::fromRgb(ui->lblGeometry->colour));
  ui->lblCompartmentColour->setPalette(sample_palette);
  QString compID = ui->lblGeometry->compartmentID;
  ui->listSpecies->clear();
  ui->btnChangeCompartment->setText(compID);
  // update species info to this compartment
  ui->listSpecies->insertItems(0, sbml_doc.species[compID]);
  // show species tab
  ui->tabMain->setCurrentIndex(1);
}

void MainWindow::on_chkEnableSpatial_stateChanged(int arg1) {
  ui->grpSpatial->setEnabled(arg1);
}

void MainWindow::on_chkShowSpatialAdvanced_stateChanged(int arg1) {
  ui->grpSpatialAdavanced->setEnabled(arg1);
}

void MainWindow::on_listSpecies_currentTextChanged(const QString &currentText) {
  if (currentText.size() > 0) {
    qDebug() << currentText;
    auto *spec = sbml_doc.model->getSpecies(qPrintable(currentText));
    ui->txtInitialConcentration->setText(
        QString::number(spec->getInitialConcentration()));
  }
}

void MainWindow::on_btnChangeCompartment_triggered(QAction *arg1) {
  QString compID = arg1->text();
  ui->lblGeometry->colour_to_comp[ui->lblGeometry->colour] = compID;
  ui->lblGeometry->compartmentID = compID;
  on_lblGeometry_mouseClicked();
}

void MainWindow::on_listReactions_currentTextChanged(
    const QString &currentText) {
  ui->listProducts->clear();
  ui->listReactants->clear();
  ui->listReactionParams->clear();
  ui->lblReactionRate->clear();
  if (currentText.size() > 0) {
    qDebug() << currentText;
    const auto *reac = sbml_doc.model->getReaction(qPrintable(currentText));
    for (unsigned i = 0; i < reac->getNumProducts(); ++i) {
      ui->listProducts->addItem(reac->getProduct(i)->getSpecies().c_str());
    }
    for (unsigned i = 0; i < reac->getNumReactants(); ++i) {
      ui->listReactants->addItem(reac->getReactant(i)->getSpecies().c_str());
    }
    for (unsigned i = 0; i < reac->getKineticLaw()->getNumParameters(); ++i) {
      ui->listReactionParams->addItem(
          reac->getKineticLaw()->getParameter(i)->getId().c_str());
    }
    ui->lblReactionRate->setText(reac->getKineticLaw()->getFormula().c_str());
  }
}

void MainWindow::on_btnSimulate_clicked() {
  // do simple simulation of model
  // compile reaction expressions
  simulate sim(sbml_doc);
  sim.compile_reactions();
  // set initial concentrations
  for (unsigned int i = 0; i < sbml_doc.model->getNumSpecies(); ++i) {
    sim.species_values[i] =
        sbml_doc.model->getSpecies(i)->getInitialConcentration();
  }
  // generate vector of resulting concentrations at each timestep
  sim.euler_timestep(0.0);
  QVector<double> time;
  double t = 0;
  double dt = ui->txtSimDt->text().toDouble();
  std::vector<QVector<double>> conc(sim.species_values.size());
  time.push_back(t);
  for (std::size_t i = 0; i < sim.species_values.size(); ++i) {
    conc[i].push_back(sim.species_values[i]);
  }
  for (int i = 0;
       i < static_cast<int>(ui->txtSimLength->text().toDouble() / dt); ++i) {
    // do an euler integration timestep
    sim.euler_timestep(dt);
    t += dt;
    time.push_back(t);
    for (std::size_t i = 0; i < sim.species_values.size(); ++i) {
      conc[i].push_back(sim.species_values[i]);
    }
  }
  // plot results
  ui->pltPlot->clearGraphs();
  std::vector<QColor> col{{0, 0, 0},       {230, 25, 75},   {60, 180, 75},
                          {255, 225, 25},  {0, 130, 200},   {245, 130, 48},
                          {145, 30, 180},  {70, 240, 240},  {240, 50, 230},
                          {210, 245, 60},  {250, 190, 190}, {0, 128, 128},
                          {230, 190, 255}, {170, 110, 40},  {255, 250, 200},
                          {128, 0, 0},     {170, 255, 195}, {128, 128, 0},
                          {255, 215, 180}, {0, 0, 128},     {128, 128, 128}};
  ui->pltPlot->legend->setVisible(true);
  for (std::size_t i = 0; i < sim.species_values.size(); ++i) {
    ui->pltPlot->addGraph();
    ui->pltPlot->graph(static_cast<int>(i))->setData(time, conc[i]);
    ui->pltPlot->graph(static_cast<int>(i))->setPen(col[i]);
    ui->pltPlot->graph(static_cast<int>(i))
        ->setName(sbml_doc.speciesID[i].c_str());
  }
  ui->pltPlot->xAxis->setLabel("time");
  ui->pltPlot->xAxis->setRange(time.front(), time.back());
  ui->pltPlot->yAxis->setLabel("concentration");
  ui->pltPlot->yAxis->setRange(0, 2);
  ui->pltPlot->replot();
}

void MainWindow::on_listFunctions_currentTextChanged(
    const QString &currentText) {
  ui->listFunctionParams->clear();
  ui->lblFunctionDef->clear();
  if (currentText.size() > 0) {
    qDebug() << currentText;
    const auto *func =
        sbml_doc.model->getFunctionDefinition(qPrintable(currentText));
    for (unsigned i = 0; i < func->getNumArguments(); ++i) {
      ui->listFunctionParams->addItem(
          libsbml::SBML_formulaToL3String(func->getArgument(i)));
    }
    ui->lblFunctionDef->setText(
        libsbml::SBML_formulaToL3String(func->getBody()));
  }
}
