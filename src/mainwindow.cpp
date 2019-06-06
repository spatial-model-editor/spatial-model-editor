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

  // <debug>
  // for debugging convenience: import a model and an image on startup
  sbml_doc.importSBMLFile("ABtoC.xml");
  sbml_doc.importGeometryFromImage("two-blobs-100x100.bmp");
  ui->lblGeometry->setImage(sbml_doc.getCompartmentImage());
  update_ui();
  // </debug>
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
  // update raw XML display
  ui->txtSBML->setText(sbml_doc.xml);

  // update list of compartments
  ui->listCompartments->clear();
  ui->listCompartments->insertItems(0, sbml_doc.compartments);

  // update list of reactions
  ui->listReactions->clear();
  ui->listReactions->insertItems(0, sbml_doc.reactions);

  // update list of functions
  ui->listFunctions->clear();
  ui->listFunctions->insertItems(0, sbml_doc.functions);

  // update tree list of species
  ui->listSpecies->clear();
  QList<QTreeWidgetItem *> items;
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
    qDebug() << item->text(column);
    // display species information
    auto *spec = sbml_doc.model->getSpecies(qPrintable(item->text(column)));
    ui->txtInitialConcentration->setText(
        QString::number(spec->getInitialConcentration()));
    if ((spec->isSetConstant() && spec->getConstant()) ||
        (spec->isSetBoundaryCondition() && spec->getBoundaryCondition())) {
      ui->chkSpeciesIsConstant->setCheckState(Qt::CheckState::Checked);
    } else {
      ui->chkSpeciesIsConstant->setCheckState(Qt::CheckState::Unchecked);
    }
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
  ui->lblGeometry->setImage(images[dataIndex]);
}

void MainWindow::on_btnChangeCompartment_clicked() {
  waiting_for_compartment_choice = true;
}

void MainWindow::on_listCompartments_currentTextChanged(
    const QString &currentText) {
  ui->txtCompartmentSize->clear();
  if (currentText.size() > 0) {
    qDebug() << currentText;
    const auto *comp = sbml_doc.model->getCompartment(qPrintable(currentText));
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
      ui->lblGeometry->setMaskColour(col);
      QPixmap pixmap = QPixmap::fromImage(ui->lblGeometry->getMask());
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
  qDebug() << index;
  if (index == 0) {
    // geometry tab
    ui->lblGeometry->setImage(sbml_doc.getCompartmentImage());
    ui->hslideTime->setEnabled(false);
  } else if (index == 4) {
    // simulate tab
    ui->hslideTime->setEnabled(true);
    ui->hslideTime->setValue(0);
    on_hslideTime_valueChanged(0);
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

void MainWindow::on_btnSimulate_clicked() {
  // simple 2d spatial simulation
  images.clear();

  // for now only simulate first compartment
  auto comp = sbml_doc.compartments[0];

  Field &species_field = sbml_doc.field;
  // compile reaction expressions
  Simulate sim(sbml_doc);
  sim.compile_reactions(sbml_doc.speciesID);
  // do euler integration
  QVector<double> time;
  std::vector<QVector<double>> conc(sim.species_values.size());
  for (std::size_t i = 0; i < sim.species_values.size(); ++i) {
    conc[i].push_back(species_field.get_mean_concentration(i));
  }
  double t = 0;
  double dt = ui->txtSimDt->text().toDouble();
  for (int i = 0;
       i < static_cast<int>(ui->txtSimLength->text().toDouble() / dt); ++i) {
    t += dt;
    sim.timestep_2d_euler(species_field, dt);
    images.push_back(species_field.getConcentrationImage().copy());
    for (std::size_t k = 0; k < sim.species_values.size(); ++k) {
      conc[k].push_back(species_field.get_mean_concentration(k));
    }
    time.push_back(t);
  }
  // plot results
  ui->pltPlot->clearGraphs();
  ui->pltPlot->setInteraction(QCP::iRangeDrag, true);
  ui->pltPlot->setInteraction(QCP::iRangeZoom, true);
  ui->pltPlot->setInteraction(QCP::iSelectPlottables, true);
  ui->pltPlot->legend->setVisible(true);
  for (std::size_t i = 0; i < sim.species_values.size(); ++i) {
    ui->pltPlot->addGraph();
    ui->pltPlot->graph(static_cast<int>(i))->setData(time, conc[i]);
    ui->pltPlot->graph(static_cast<int>(i))
        ->setPen(species_field.speciesColour[i]);
    ui->pltPlot->graph(static_cast<int>(i))
        ->setName(sbml_doc.speciesID[i].c_str());
  }
  ui->pltPlot->xAxis->setLabel("time");
  ui->pltPlot->xAxis->setRange(time.front(), time.back());
  ui->pltPlot->yAxis->setLabel("concentration");
  ui->pltPlot->yAxis->setRange(
      0, 1.5 * (*std::max_element(conc[0].cbegin(), conc[0].cend())));
  ui->pltPlot->replot();
  // enable slider to choose time to display
  ui->hslideTime->setEnabled(true);
  ui->hslideTime->setMinimum(0);
  ui->hslideTime->setMaximum(time.size() - 1);
  ui->hslideTime->setValue(time.size() - 1);
}
