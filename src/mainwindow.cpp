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

  connect(ui->pltPlot,
          SIGNAL(plottableClick(QCPAbstractPlottable *, int, QMouseEvent *)),
          this, SLOT(on_graphClicked(QCPAbstractPlottable *, int)));
  ui->tabMain->setCurrentIndex(0);
  on_tabMain_currentChanged(0);
}

MainWindow::~MainWindow() { delete ui; }

void MainWindow::on_action_About_triggered() {
  QMessageBox msgBox;
  msgBox.setWindowTitle("About");
  QString info("Spatial Model Editor\n");
  info.append("github.com/lkeegan/spatial-model-editor\n\n");
  info.append("Included libraries:\n");
  info.append("\nQt5:\t\t");
  info.append(QT_VERSION_STR);
  info.append("\nlibSBML:\t\t");
  info.append(libsbml::getLibSBMLDottedVersion());
  info.append("\nQCustomPlot:\t2.0.1");
  for (const auto &dep : {"expat", "libxml", "xerces-c", "bzip2", "zip"}) {
    if (libsbml::isLibSBMLCompiledWith(dep) != 0) {
      info.append("\n");
      info.append(dep);
      info.append(":\t\t");
      info.append(libsbml::getLibSBMLDependencyVersionOf(dep));
    }
  }
  info.append("\nC++ Mathematical Expression  Toolkit Library (ExprTk)");
  msgBox.setText(info);
  msgBox.exec();
}

void MainWindow::on_actionE_xit_triggered() { QApplication::quit(); }

void MainWindow::on_action_Open_SBML_file_triggered() {
  // load SBML file
  QString filename = QFileDialog::getOpenFileName(this, "Open SBML file", "",
                                                  "SBML file (*.xml)");
  if (!filename.isEmpty()) {
    sbml_doc.importSBMLFile(qPrintable(filename));
    if (sbml_doc.isValid) {
      update_ui();
    }
  }
}

void MainWindow::update_ui() {
  // update list of compartments
  ui->listCompartments->clear();
  ui->listCompartments->insertItems(0, sbml_doc.compartments);

  // update list of membranes
  ui->listMembranes->clear();
  ui->listMembranes->insertItems(0, sbml_doc.membranes);

  // update list of functions
  ui->listFunctions->clear();
  ui->listFunctions->insertItems(0, sbml_doc.functions);

  // update tree list of species
  ui->listSpecies->clear();
  for (auto c : sbml_doc.compartments) {
    // add compartments as top level items
    QTreeWidgetItem *comp =
        new QTreeWidgetItem(ui->listSpecies, QStringList({c}));
    ui->listSpecies->addTopLevelItem(comp);
    for (auto s : sbml_doc.species[c]) {
      // add each species as child of compartment
      comp->addChild(new QTreeWidgetItem(comp, QStringList({s})));
    }
  }
  ui->listSpecies->expandAll();
}

void MainWindow::on_action_Save_SBML_file_triggered() {
  QMessageBox msgBox;
  msgBox.setText("todo");
  msgBox.exec();
}

void MainWindow::on_actionAbout_Qt_triggered() { QMessageBox::aboutQt(this); }

void MainWindow::on_actionGeometry_from_image_triggered() {
  QString filename =
      QFileDialog::getOpenFileName(this, "Import geometry from image", "",
                                   "Image Files (*.png *.jpg *.bmp)");
  sbml_doc.importGeometryFromImage(filename);
  ui->lblGeometry->setImage(sbml_doc.getCompartmentImage());
  ui->tabMain->setCurrentIndex(0);
}

void MainWindow::on_lblGeometry_mouseClicked() {
  QRgb col = ui->lblGeometry->getColour();
  if (waiting_for_compartment_choice) {
    // update compartment geometry (i.e. colour) of selected compartment to the
    // one the user just clicked on
    auto comp = ui->listCompartments->selectedItems()[0]->text();
    sbml_doc.setCompartmentColour(comp, col);
    // update display by simulating user click on listCompartments
    on_listCompartments_currentTextChanged(comp);
    waiting_for_compartment_choice = false;
  } else {
    // display compartment the user just clicked on
    auto items = ui->listCompartments->findItems(sbml_doc.getCompartmentID(col),
                                                 Qt::MatchExactly);
    if (!items.empty()) {
      ui->listCompartments->setCurrentRow(ui->listCompartments->row(items[0]));
    }
  }
}

void MainWindow::on_chkSpeciesIsSpatial_stateChanged(int arg1) {
  ui->grpSpatial->setEnabled(arg1);
  ui->btnImportConcentration->setEnabled(arg1);
}

void MainWindow::on_chkShowSpatialAdvanced_stateChanged(int arg1) {
  ui->grpSpatialAdavanced->setEnabled(arg1);
}

void MainWindow::on_listReactions_itemActivated(QTreeWidgetItem *item,
                                                int column) {
  ui->listProducts->clear();
  ui->listReactants->clear();
  ui->listReactionParams->clear();
  ui->lblReactionRate->clear();
  // if user selects a species (i.e. an item with a parent)
  if ((item != nullptr) && (item->parent() != nullptr)) {
    qDebug("ui::listReactions :: Reaction '%s' selected",
           item->text(column).toStdString().c_str());
    // display species information
    const auto *reac =
        sbml_doc.model->getReaction(item->text(column).toStdString());
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

void MainWindow::on_listReactions_itemClicked(QTreeWidgetItem *item,
                                              int column) {
  on_listReactions_itemActivated(item, column);
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

void MainWindow::on_listSpecies_itemActivated(QTreeWidgetItem *item,
                                              int column) {
  // if user selects a species (i.e. an item with a parent)
  if ((item != nullptr) && (item->parent() != nullptr)) {
    qDebug("ui::listSpecies :: Species '%s' selected",
           item->text(column).toStdString().c_str());
    // display species information
    auto *spec = sbml_doc.model->getSpecies(item->text(column).toStdString());
    ui->txtInitialConcentration->setText(
        QString::number(spec->getInitialConcentration()));
    if ((spec->isSetConstant() && spec->getConstant()) ||
        (spec->isSetBoundaryCondition() && spec->getBoundaryCondition())) {
      ui->chkSpeciesIsConstant->setCheckState(Qt::CheckState::Checked);
    } else {
      ui->chkSpeciesIsConstant->setCheckState(Qt::CheckState::Unchecked);
    }
    ui->lblGeometryStatus->setText("Species concentration:");
    ui->lblGeometry->setImage(
        sbml_doc.getConcentrationImage(item->text(column)));
  }
}

void MainWindow::on_listSpecies_itemClicked(QTreeWidgetItem *item, int column) {
  on_listSpecies_itemActivated(item, column);
}

void MainWindow::on_graphClicked(QCPAbstractPlottable *plottable,
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

void MainWindow::on_btnChangeCompartment_clicked() {
  waiting_for_compartment_choice = true;
}

void MainWindow::on_listCompartments_currentTextChanged(
    const QString &currentText) {
  ui->txtCompartmentSize->clear();
  if (currentText.size() > 0) {
    qDebug() << currentText;
    const auto *comp =
        sbml_doc.model->getCompartment(currentText.toStdString());
    ui->txtCompartmentSize->setText(QString::number(comp->getSize()));
    QRgb col = sbml_doc.getCompartmentColour(currentText);
    qDebug() << qAlpha(col);
    if (col == 0) {
      // null (transparent white) RGB colour: compartment does not have
      // an assigned colour in the image
      ui->lblCompartmentColour->setPalette(QPalette());
      ui->lblCompartmentColour->setText("none");
      ui->lblCompShape->setPixmap(QPixmap());
      ui->lblCompShape->setText("none");
    } else {
      // update colour box
      QPalette palette;
      palette.setColor(QPalette::Window, QColor::fromRgb(col));
      ui->lblCompartmentColour->setPalette(palette);
      ui->lblCompartmentColour->setText("");
      // update image mask
      QPixmap pixmap = QPixmap::fromImage(
          sbml_doc.mapCompIdToGeometry.at(currentText).getCompartmentImage());
      ui->lblCompShape->setPixmap(pixmap);
      ui->lblCompShape->setText("");
    }
  }
}

void MainWindow::on_hslideTime_valueChanged(int value) {
  qDebug() << value;
  if (images.size() > value) {
    ui->lblGeometry->setImage(images[value]);
  }
}

void MainWindow::on_tabMain_currentChanged(int index) {
  qDebug("Tab changed to %d", index);
  ui->hslideTime->setEnabled(false);
  ui->hslideTime->setVisible(false);
  switch (index) {
    case 0:
      // geometry tab
      ui->lblGeometry->setImage(sbml_doc.getCompartmentImage());
      ui->lblGeometryStatus->setText("Compartment Geometry:");
      break;
    case 1:
      // membranes tab
      ui->lblMembraneShape->clear();
      ui->listMembranes->clear();
      ui->listMembranes->addItems(sbml_doc.membranes);
      ui->lblGeometry->setImage(sbml_doc.getCompartmentImage());
      ui->lblGeometryStatus->setText("Compartment Geometry:");
      if (ui->listMembranes->count() > 0) {
        ui->listMembranes->setCurrentRow(0);
      }
      break;
    case 2:
      // species tab
      on_listSpecies_itemActivated(ui->listSpecies->currentItem(),
                                   ui->listSpecies->currentColumn());
      break;
    case 3:
      // reactions tab
      // update list of reactions
      ui->listReactions->clear();
      for (auto iter = sbml_doc.reactions.cbegin();
           iter != sbml_doc.reactions.cend(); ++iter) {
        // add compartments as top level items
        QTreeWidgetItem *comp =
            new QTreeWidgetItem(ui->listReactions, QStringList({iter->first}));
        ui->listReactions->addTopLevelItem(comp);
        for (auto s : iter->second) {
          // add each species as child of compartment
          comp->addChild(new QTreeWidgetItem(comp, QStringList({s})));
        }
      }
      ui->listReactions->expandAll();
      break;
    case 4:
      // functions tab
    case 5:
      // simulate tab
      ui->lblGeometryStatus->setText("Simulation concentration:");
      ui->hslideTime->setVisible(true);
      ui->hslideTime->setEnabled(true);
      ui->hslideTime->setValue(0);
      on_hslideTime_valueChanged(0);
      break;
    case 6:
      // SBML tab
      ui->txtSBML->setText(sbml_doc.getXml());
  }
}

void MainWindow::on_btnImportConcentration_clicked() {
  auto spec = ui->listSpecies->selectedItems()[0]->text(0);
  qDebug() << spec;
  QString filename = QFileDialog::getOpenFileName(
      this, "Import species concentration from image", "",
      "Image Files (*.png *.jpg *.bmp)");
  sbml_doc.importConcentrationFromImage(spec, filename);
  ui->lblGeometry->setImage(sbml_doc.getConcentrationImage(spec));
}

void MainWindow::on_listMembranes_currentTextChanged(
    const QString &currentText) {
  if (currentText.size() > 0) {
    qDebug() << currentText;
    // update image
    QPixmap pixmap = QPixmap::fromImage(sbml_doc.getMembraneImage(currentText));
    ui->lblMembraneShape->setPixmap(pixmap);
  }
}

void MainWindow::on_btnSimulate_clicked() {
  // simple 2d spatial simulation

  //
  std::vector<Simulate> sim;
  // create a Simulate object for each compartment
  for (const auto &compartmentID : sbml_doc.compartments) {
    Field &species_field = sbml_doc.mapCompIdToField[compartmentID];
    // compile reaction expressions
    std::vector<std::string> tmpSpeciesID;
    tmpSpeciesID.reserve(
        static_cast<std::size_t>(sbml_doc.species[compartmentID].size()));
    for (const auto &s : sbml_doc.species[compartmentID]) {
      tmpSpeciesID.push_back(s.toStdString());
    }
    sim.emplace_back(&sbml_doc, &species_field);
    sim.back().compile_reactions();
  }

  // do euler integration
  // only plot/display first compartment for now...
  images.clear();
  QVector<double> time{0};
  std::vector<QVector<double>> conc(sim[0].species_values.size());
  for (std::size_t i = 0; i < sim[0].species_values.size(); ++i) {
    conc[i].push_back(sim[0].field->getMeanConcentration(i));
  }
  double t = 0;
  double dt = ui->txtSimDt->text().toDouble();
  for (int i = 0;
       i < static_cast<int>(ui->txtSimLength->text().toDouble() / dt); ++i) {
    t += dt;
    for (auto &s : sim) {
      s.timestep_2d_euler(dt);
    }
    images.push_back(sim[0].field->getConcentrationImage().copy());
    for (std::size_t k = 0; k < sim[0].species_values.size(); ++k) {
      conc[k].push_back(sim[0].field->getMeanConcentration(k));
    }
    time.push_back(t);
    ui->lblGeometry->setImage(images.back());
    ui->lblGeometry->repaint();
  }

  // plot results
  ui->pltPlot->clearGraphs();
  ui->pltPlot->setInteraction(QCP::iRangeDrag, true);
  ui->pltPlot->setInteraction(QCP::iRangeZoom, true);
  ui->pltPlot->setInteraction(QCP::iSelectPlottables, true);
  ui->pltPlot->legend->setVisible(true);
  for (std::size_t i = 0; i < sim[0].species_values.size(); ++i) {
    auto *graph = ui->pltPlot->addGraph();
    graph->setData(time, conc[i]);
    graph->setPen(sim[0].field->speciesColour[i]);
    graph->setName(sim[0].field->speciesID[i].c_str());
  }
  ui->pltPlot->xAxis->setLabel("time");
  ui->pltPlot->yAxis->setLabel("concentration");
  ui->pltPlot->xAxis->setRange(time.front(), time.back());
  double ymax = *std::max_element(conc[0].cbegin(), conc[0].cend());
  for (std::size_t i = 1; i < conc.size(); ++i) {
    ymax = std::max(ymax, *std::max_element(conc[i].cbegin(), conc[i].cend()));
  }
  ui->pltPlot->yAxis->setRange(0, 1.2 * ymax);
  ui->pltPlot->replot();

  // enable slider to choose time to display
  ui->hslideTime->setEnabled(true);
  ui->hslideTime->setMinimum(0);
  ui->hslideTime->setMaximum(time.size() - 1);
  ui->hslideTime->setValue(time.size() - 1);
}
