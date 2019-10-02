#include "mainwindow.hpp"

#include <QErrorMessage>
#include <QFileDialog>
#include <QInputDialog>
#include <QMessageBox>
#include <QString>
#include <QStringListModel>
#include <sstream>

#include "dialogdimensions.hpp"
#include "dune.hpp"
#include "logger.hpp"
#include "mesh.hpp"
#include "simulate.hpp"
#include "ui_mainwindow.h"
#include "version.hpp"

static void selectFirstChild(QTreeWidget *tree) {
  auto *firstParent = tree->topLevelItem(0);
  if (firstParent != nullptr) {
    auto *firstChild = firstParent->child(0);
    if (firstChild != nullptr) {
      tree->setCurrentItem(firstChild);
    }
  }
}

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent), ui(new Ui::MainWindow) {
  ui->setupUi(this);

  shortcutStopSimulation = new QShortcut(this);
  shortcutStopSimulation->setKey(Qt::CTRL + Qt::Key_C);

  shortcutSetMathBackend = new QShortcut(this);
  shortcutSetMathBackend->setKey(Qt::CTRL + Qt::SHIFT + Qt::Key_B);

  shortcutToggleDune = new QShortcut(this);
  shortcutToggleDune->setKey(Qt::CTRL + Qt::SHIFT + Qt::Key_D);

  lblSpeciesColourPixmap = QPixmap(1, 1);
  lblCompartmentColourPixmap = QPixmap(1, 1);

  statusBarPermanentMessage = new QLabel(QString(), this);
  ui->statusBar->addWidget(statusBarPermanentMessage);

  setupConnections();

  enableTabs();
  ui->tabMain->setCurrentIndex(0);
  ui->tabCompartmentGeometry->setCurrentIndex(0);
  tabMain_currentChanged(0);
}

void MainWindow::setupConnections() {
  // temp:
  connect(shortcutToggleDune, &QShortcut::activated, this,
          [this]() { useDuneSimulator = !useDuneSimulator; });

  // tab bar
  connect(ui->tabMain, &QTabWidget::currentChanged, this,
          &MainWindow::tabMain_currentChanged);

  // menu actions
  connect(ui->action_Open_SBML_file, &QAction::triggered, this,
          &MainWindow::action_Open_SBML_file_triggered);

  connect(ui->menuOpen_example_SBML_file, &QMenu::triggered, this,
          &MainWindow::menuOpen_example_SBML_file_triggered);

  connect(ui->action_Save_SBML_file, &QAction::triggered, this,
          &MainWindow::action_Save_SBML_file_triggered);

  connect(ui->actionExport_Dune_ini_file, &QAction::triggered, this,
          &MainWindow::actionExport_Dune_ini_file_triggered);

  connect(ui->actionE_xit, &QAction::triggered, this,
          []() { QApplication::quit(); });

  connect(ui->actionGeometry_from_image, &QAction::triggered, this,
          &MainWindow::actionGeometry_from_image_triggered);

  connect(ui->menuExample_geometry_image, &QMenu::triggered, this,
          &MainWindow::menuExample_geometry_image_triggered);

  connect(ui->action_Set_geometry_dimensions, &QAction::triggered, this,
          &MainWindow::action_Set_geometry_dimensions_triggered);

  connect(ui->action_What_s_this, &QAction::triggered, this,
          []() { QWhatsThis::enterWhatsThisMode(); });

  connect(ui->actionOnline_Documentation, &QAction::triggered, this, []() {
    QDesktopServices::openUrl(
        QUrl(QString("https://spatial-model-editor.readthedocs.io")));
  });

  connect(ui->action_About, &QAction::triggered, this,
          &MainWindow::action_About_triggered);

  connect(ui->actionAbout_Qt, &QAction::triggered, this,
          [this]() { QMessageBox::aboutQt(this); });

  // geometry
  connect(ui->lblGeometry, &QLabelMouseTracker::mouseClicked, this,
          &MainWindow::lblGeometry_mouseClicked);

  connect(ui->btnChangeCompartment, &QPushButton::clicked, this,
          &MainWindow::btnChangeCompartment_clicked);

  connect(ui->tabCompartmentGeometry, &QTabWidget::currentChanged, this,
          &MainWindow::tabCompartmentGeometry_currentChanged);

  connect(ui->spinBoundaryIndex, qOverload<int>(&QSpinBox::valueChanged), this,
          &MainWindow::spinBoundaryIndex_valueChanged);

  connect(ui->spinMaxBoundaryPoints, qOverload<int>(&QSpinBox::valueChanged),
          this, &MainWindow::spinMaxBoundaryPoints_valueChanged);

  connect(ui->spinMaxTriangleArea, qOverload<int>(&QSpinBox::valueChanged),
          this, &MainWindow::spinMaxTriangleArea_valueChanged);

  connect(ui->listCompartments, &QListWidget::currentTextChanged, this,
          &MainWindow::listCompartments_currentTextChanged);

  connect(ui->listCompartments, &QListWidget::itemDoubleClicked, this,
          &MainWindow::listCompartments_itemDoubleClicked);

  // membranes
  connect(ui->listMembranes, &QListWidget::currentTextChanged, this,
          &MainWindow::listMembranes_currentTextChanged);

  // species
  connect(ui->listSpecies, &QTreeWidget::currentItemChanged, this,
          &MainWindow::listSpecies_currentItemChanged);

  connect(ui->chkSpeciesIsSpatial, &QCheckBox::toggled, this,
          &MainWindow::chkSpeciesIsSpatial_toggled);

  connect(ui->chkSpeciesIsConstant, &QCheckBox::toggled, this,
          &MainWindow::chkSpeciesIsConstant_toggled);

  connect(ui->radInitialConcentrationUniform, &QRadioButton::toggled, this,
          &MainWindow::radInitialConcentration_toggled);

  connect(ui->txtInitialConcentration, &QLineEdit::editingFinished, this,
          &MainWindow::txtInitialConcentration_editingFinished);

  connect(ui->radInitialConcentrationVarying, &QRadioButton::toggled, this,
          &MainWindow::radInitialConcentration_toggled);

  connect(ui->btnAnalyticConcentration, &QPushButton::clicked, this,
          &MainWindow::btnAnalyticConcentration_clicked);

  connect(ui->btnImportConcentration, &QPushButton::clicked, this,
          &MainWindow::btnImportConcentration_clicked);

  connect(ui->cmbImportExampleConcentration, &QComboBox::currentTextChanged,
          this, &MainWindow::cmbImportExampleConcentration_currentTextChanged);

  connect(ui->txtDiffusionConstant, &QLineEdit::editingFinished, this,
          &MainWindow::txtDiffusionConstant_editingFinished);

  connect(ui->btnChangeSpeciesColour, &QPushButton::clicked, this,
          &MainWindow::btnChangeSpeciesColour_clicked);

  // reactions
  connect(ui->listReactions, &QTreeWidget::currentItemChanged, this,
          &MainWindow::listReactions_currentItemChanged);

  // functions
  connect(ui->listFunctions, &QListWidget::currentTextChanged, this,
          &MainWindow::listFunctions_currentTextChanged);

  // simulate
  connect(ui->btnSimulate, &QPushButton::clicked, this,
          &MainWindow::btnSimulate_clicked);

  connect(ui->btnResetSimulation, &QPushButton::clicked, this,
          &MainWindow::btnResetSimulation_clicked);

  connect(shortcutStopSimulation, &QShortcut::activated, this,
          [this]() { isSimulationRunning = false; });

  connect(shortcutSetMathBackend, &QShortcut::activated, this,
          &MainWindow::setMathBackend);

  connect(ui->pltPlot, &QCustomPlot::plottableClick, this,
          &MainWindow::graphClicked);

  connect(ui->hslideTime, &QSlider::valueChanged, this,
          &MainWindow::hslideTime_valueChanged);
}

void MainWindow::setMathBackend() {
  bool ok;
  QStringList items;
  items << simulate::strBackend(simulate::BACKEND::EXPRTK).c_str();
  items << simulate::strBackend(simulate::BACKEND::SYMENGINE).c_str();
  items << simulate::strBackend(simulate::BACKEND::SYMENGINE_LLVM).c_str();
  int currentIndex = 0;
  if (simMathBackend == simulate::BACKEND::SYMENGINE) {
    currentIndex = 1;
  } else if (simMathBackend == simulate::BACKEND::SYMENGINE_LLVM) {
    currentIndex = 2;
  }
  QString item = QInputDialog::getItem(this, "Select simulation math backend",
                                       "Simulation math backend:", items,
                                       currentIndex, false, &ok);
  if (ok && !item.isEmpty()) {
    if (item == items[0]) {
      simMathBackend = simulate::BACKEND::EXPRTK;
    } else if (item == items[1]) {
      simMathBackend = simulate::BACKEND::SYMENGINE;
    } else if (item == items[2]) {
      simMathBackend = simulate::BACKEND::SYMENGINE_LLVM;
    }

    SPDLOG_INFO("Setting math backend to {}", item);
  }
}

void MainWindow::updateSpeciesDisplaySelect() {
  std::vector<std::string> species;
  for (auto *act : ui->btnSpeciesDisplaySelect->menu()->actions()) {
    if (act->isChecked()) {
      qDebug("show %s", act->text().toStdString().c_str());
      species.push_back(act->text().toStdString());
      // todo
    }
  }
}

void MainWindow::tabMain_currentChanged(int index) {
  enum TabIndex {
    GEOMETRY = 0,
    MEMBRANES = 1,
    SPECIES = 2,
    REACTIONS = 3,
    FUNCTIONS = 4,
    SIMULATE = 5,
    SBML = 6,
    DUNE = 7,
    GMSH = 8
  };
  ui->tabMain->setWhatsThis(ui->tabMain->tabWhatsThis(index));
  SPDLOG_DEBUG("Tab changed to {} [{}]", index,
               ui->tabMain->tabText(index).toStdString());
  ui->hslideTime->setEnabled(false);
  ui->hslideTime->setVisible(false);
  ui->btnSpeciesDisplaySelect->setEnabled(false);
  ui->btnSpeciesDisplaySelect->setVisible(false);
  switch (index) {
    case TabIndex::GEOMETRY:
      tabMain_updateGeometry();
      break;
    case TabIndex::MEMBRANES:
      tabMain_updateMembranes();
      break;
    case TabIndex::SPECIES:
      tabMain_updateSpecies();
      break;
    case TabIndex::REACTIONS:
      tabMain_updateReactions();
      break;
    case TabIndex::FUNCTIONS:
      tabMain_updateFunctions();
      break;
    case TabIndex::SIMULATE:
      tabMain_updateSimulate();
      break;
    case TabIndex::SBML:
      ui->txtSBML->setText(sbmlDoc.getXml());
      break;
    case TabIndex::DUNE:
      ui->txtDUNE->setText(dune::DuneConverter(sbmlDoc).getIniFile());
      break;
    case TabIndex::GMSH:
      ui->txtGMSH->setText(sbmlDoc.mesh.getGMSH());
      break;
    default:
      qFatal("ui::tabMain :: Errror: Tab index %d not valid", index);
  }
}

void MainWindow::tabMain_updateGeometry() {
  ui->listCompartments->clear();
  ui->listCompartments->insertItems(0, sbmlDoc.compartments);
  if (ui->listCompartments->count() > 0) {
    ui->listCompartments->setCurrentRow(0);
  }
  ui->lblGeometry->setImage(sbmlDoc.getCompartmentImage());
  ui->lblGeometryStatus->setText("Compartment Geometry:");
}

void MainWindow::tabMain_updateMembranes() {
  ui->lblMembraneShape->clear();
  ui->listMembranes->clear();
  ui->listMembranes->addItems(sbmlDoc.membranes);
  ui->lblGeometry->setImage(sbmlDoc.getCompartmentImage());
  ui->lblGeometryStatus->setText("Compartment Geometry:");
  if (ui->listMembranes->count() > 0) {
    ui->listMembranes->setCurrentRow(0);
  }
}

void MainWindow::tabMain_updateSpecies() {
  // clear any changes to species concentrations by simulations
  // reset all fields to their initial values
  for (auto &field : sbmlDoc.mapSpeciesIdToField) {
    field.second.conc = field.second.init;
  }
  // update tree list of species
  auto *ls = ui->listSpecies;
  ls->clear();
  for (auto c : sbmlDoc.compartments) {
    // add compartments as top level items
    QTreeWidgetItem *comp = new QTreeWidgetItem(ls, QStringList({c}));
    ls->addTopLevelItem(comp);
    for (auto s : sbmlDoc.species[c]) {
      // add each species as child of compartment
      comp->addChild(new QTreeWidgetItem(comp, QStringList({s})));
    }
  }
  ls->expandAll();
  selectFirstChild(ls);
}

void MainWindow::tabMain_updateReactions() {
  ui->lblGeometryStatus->setText("Reaction location:");
  // update tree list of reactions
  auto *ls = ui->listReactions;
  ls->clear();
  for (auto iter = sbmlDoc.reactions.cbegin(); iter != sbmlDoc.reactions.cend();
       ++iter) {
    // add compartments/membranes as top level items
    QTreeWidgetItem *comp = new QTreeWidgetItem(ls, QStringList({iter->first}));
    ls->addTopLevelItem(comp);
    for (auto s : iter->second) {
      // add each species as child of compartment
      comp->addChild(new QTreeWidgetItem(comp, QStringList({s})));
    }
  }
  ui->listReactions->expandAll();
  selectFirstChild(ls);
}

void MainWindow::tabMain_updateFunctions() {
  ui->listFunctions->clear();
  ui->listFunctions->insertItems(0, sbmlDoc.functions);
}

void MainWindow::tabMain_updateSimulate() {
  ui->lblGeometryStatus->setText("Simulation concentration:");
  ui->hslideTime->setVisible(true);
  ui->hslideTime->setEnabled(true);
  ui->hslideTime->setValue(0);
  ui->btnSpeciesDisplaySelect->setEnabled(true);
  ui->btnSpeciesDisplaySelect->setVisible(true);
  if (images.empty()) {
    simulate::Simulate sim(&sbmlDoc);
    for (const auto &compartmentID : sbmlDoc.compartments) {
      sim.addCompartment(&sbmlDoc.mapCompIdToGeometry.at(compartmentID));
    }
    ui->lblGeometry->setImage(sim.getConcentrationImage());
  }
  hslideTime_valueChanged(0);
}

void MainWindow::enableTabs() {
  bool enable = sbmlDoc.isValid && sbmlDoc.hasGeometry;
  for (int i = 1; i < ui->tabMain->count(); ++i) {
    ui->tabMain->setTabEnabled(i, enable);
  }
}

void MainWindow::action_Open_SBML_file_triggered() {
  QString filename = QFileDialog::getOpenFileName(
      this, "Open SBML file", "", "SBML file (*.xml)", nullptr,
      QFileDialog::Option::DontUseNativeDialog);
  if (!filename.isEmpty()) {
    sbmlDoc.importSBMLFile(filename.toStdString());
    if (sbmlDoc.isValid) {
      ui->tabMain->setCurrentIndex(0);
      tabMain_currentChanged(0);
      enableTabs();
      images.clear();
      this->setWindowTitle(
          QString("Spatial Model Editor [%1]").arg(sbmlDoc.currentFilename));
    }
  }
}

void MainWindow::menuOpen_example_SBML_file_triggered(QAction *action) {
  QString filename =
      QString(":/models/%1.xml").arg(action->text().remove(0, 1));
  QFile f(filename);
  if (!f.open(QIODevice::ReadOnly)) {
    SPDLOG_WARN("failed to open built-in file: {}", filename);
  }
  sbmlDoc.importSBMLString(f.readAll().toStdString());
  ui->tabMain->setCurrentIndex(0);
  tabMain_currentChanged(0);
  enableTabs();
  images.clear();
}

void MainWindow::action_Save_SBML_file_triggered() {
  QString filename = QFileDialog::getSaveFileName(
      this, "Save SBML file", sbmlDoc.currentFilename, "SBML file (*.xml)",
      nullptr, QFileDialog::Option::DontUseNativeDialog);
  if (!filename.isEmpty()) {
    if (filename.right(4) != ".xml") {
      filename.append(".xml");
    }
    sbmlDoc.exportSBMLFile(filename.toStdString());
  }
}

void MainWindow::actionExport_Dune_ini_file_triggered() {
  QString filename = QFileDialog::getSaveFileName(
      this, "Export DUNE ini file", "", "DUNE ini file (*.ini)", nullptr,
      QFileDialog::Option::DontUseNativeDialog);
  if (!filename.isEmpty()) {
    if (filename.right(4) != ".ini") {
      filename.append(".ini");
    }
    QString iniFileString = dune::DuneConverter(sbmlDoc).getIniFile();
    QFile f(filename);
    if (f.open(QIODevice::ReadWrite | QIODevice::Text)) {
      f.write(dune::DuneConverter(sbmlDoc).getIniFile().toUtf8());
    }
    // also export gmsh file `grid.msh` in the same dir
    QString dir = QFileInfo(filename).absolutePath();
    QString meshFilename = QDir(dir).filePath("grid.msh");
    QFile f2(meshFilename);
    if (f2.open(QIODevice::ReadWrite | QIODevice::Text)) {
      f2.write(sbmlDoc.mesh.getGMSH().toUtf8());
    }
  }
}

void MainWindow::actionGeometry_from_image_triggered() {
  QString filename = QFileDialog::getOpenFileName(
      this, "Import geometry from image", "",
      "Image Files (*.png *.jpg *.bmp *.tiff)", nullptr,
      QFileDialog::Option::DontUseNativeDialog);
  if (!filename.isEmpty()) {
    sbmlDoc.importGeometryFromImage(filename);
    ui->tabMain->setCurrentIndex(0);
    tabMain_currentChanged(0);
    enableTabs();
    images.clear();
  }
}

void MainWindow::menuExample_geometry_image_triggered(QAction *action) {
  QString filename =
      QString(":/geometry/%1.png").arg(action->text().remove(0, 1));
  QFile f(filename);
  if (!f.open(QIODevice::ReadOnly)) {
    SPDLOG_WARN("failed to open built-in file: {}", filename);
  }
  sbmlDoc.importGeometryFromImage(filename);
  ui->tabMain->setCurrentIndex(0);
  tabMain_currentChanged(0);
  enableTabs();
  images.clear();
}

void MainWindow::action_Set_geometry_dimensions_triggered() {
  DialogDimensions dialog(sbmlDoc.getCompartmentImage().size(),
                          sbmlDoc.getPixelWidth());
  if (dialog.exec() == QDialog::Accepted) {
    double pixelWidth = dialog.getPixelWidth();
    SPDLOG_INFO("Set new pixel width = {}", pixelWidth);
    sbmlDoc.setPixelWidth(pixelWidth, dialog.resizeCompartments());
  }
}

void MainWindow::action_About_triggered() {
  QMessageBox msgBox;
  msgBox.setWindowTitle("About Spatial Model Editor");
  QString info(QString("<h3>Spatial Model Editor %1</h3>")
                   .arg(SPATIAL_MODEL_EDITOR_VERSION));
  info.append(
      "<p>Documentation:<ul><li><a href="
      "\"https://spatial-model-editor.readthedocs.io\">"
      "spatial-model-editor.readthedocs.io</a></p></li></ul>");
  info.append(
      "<p>Source code:<ul><li><a href="
      "\"https://github.com/lkeegan/spatial-model-editor\">"
      "github.com/lkeegan/spatial-model-editor</a></p></li></ul>");
  info.append("<p>Included libraries:<p><ul>");
  info.append(QString("<li>dune-copasi</li>"));
  info.append(QString("<li>Qt5: %1</li>").arg(QT_VERSION_STR));
  info.append(
      QString("<li>libSBML: %1</li>").arg(libsbml::getLibSBMLDottedVersion()));
  info.append("<li>QCustomPlot: 2.0.1</li>");
  info.append(QString("<li>spdlog: %1.%2.%3</li>")
                  .arg(QString::number(SPDLOG_VER_MAJOR),
                       QString::number(SPDLOG_VER_MINOR),
                       QString::number(SPDLOG_VER_PATCH)));
  info.append(QString("<li>SymEngine: 0.4.0</li>"));
  info.append(QString("<li>LLVM Core: 8.0.1</li>"));
  info.append(QString("<li>GMP: 6.1.2</li>"));
  info.append(QString("<li>Triangle: 1.6</li>"));
  info.append(QString("<li>muParser: 2.2.6.1</li>"));
  info.append(QString("<li>libTIFF: 4.0.10</li>"));
  for (const auto &dep : {"expat", "libxml", "xerces-c", "bzip2", "zip"}) {
    if (libsbml::isLibSBMLCompiledWith(dep) != 0) {
      info.append(QString("<li>%1: %2</li>")
                      .arg(dep, libsbml::getLibSBMLDependencyVersionOf(dep)));
    }
  }
  info.append(
      "<li>C++ Mathematical Expression Toolkit Library (ExprTk)</li></ul>");
  msgBox.setText(info);
  msgBox.exec();
}

void MainWindow::lblGeometry_mouseClicked(QRgb col, QPoint point) {
  if (waitingForCompartmentChoice) {
    // update compartment geometry (i.e. colour) of selected compartment to
    // the one the user just clicked on
    const auto &compartmentID =
        ui->listCompartments->selectedItems()[0]->text();
    sbmlDoc.setCompartmentColour(compartmentID, col);
    sbmlDoc.setCompartmentInteriorPoint(compartmentID, point);
    // update display by simulating user click on listCompartments
    listCompartments_currentTextChanged(compartmentID);
    SPDLOG_INFO("assigned compartment {} to colour {:x}", compartmentID, col);
    waitingForCompartmentChoice = false;
    statusBarPermanentMessage->clear();
  } else {
    // display compartment the user just clicked on
    auto items = ui->listCompartments->findItems(sbmlDoc.getCompartmentID(col),
                                                 Qt::MatchExactly);
    if (!items.empty()) {
      ui->listCompartments->setCurrentItem(items[0]);
    }
  }
}

bool MainWindow::isValidModelAndGeometry() {
  if (sbmlDoc.isValid == false) {
    SPDLOG_DEBUG("  - no SBML model");
    if (QMessageBox::question(this, "No SBML model",
                              "No valid SBML model loaded - import one now?",
                              QMessageBox::Yes | QMessageBox::No,
                              QMessageBox::Yes) == QMessageBox::Yes) {
      action_Open_SBML_file_triggered();
    }
    return false;
  }
  if (sbmlDoc.hasGeometry == false) {
    SPDLOG_DEBUG("  - no geometry image");
    if (QMessageBox::question(this, "No compartment geometry",
                              "No image of compartment geometry loaded - "
                              "import one now?",
                              QMessageBox::Yes | QMessageBox::No,
                              QMessageBox::Yes) == QMessageBox::Yes) {
      actionGeometry_from_image_triggered();
    }
    return false;
  }
  return true;
}

void MainWindow::btnChangeCompartment_clicked() {
  if (!isValidModelAndGeometry()) {
    SPDLOG_DEBUG("invalid geometry and/or model: ignoring");
    return;
  }
  SPDLOG_DEBUG("waiting for user to click on geometry image..");
  waitingForCompartmentChoice = true;
  statusBarPermanentMessage->setText(
      "Please click on the desired location on the compartment geometry "
      "image...");
}

void MainWindow::tabCompartmentGeometry_currentChanged(int index) {
  SPDLOG_DEBUG("Tab changed to {} [{}]", index,
               ui->tabCompartmentGeometry->tabText(index).toStdString());
  if (index == 1) {
    ui->spinBoundaryIndex->setMaximum(
        static_cast<int>(sbmlDoc.mesh.getBoundaries().size()) - 1);
    spinBoundaryIndex_valueChanged(ui->spinBoundaryIndex->value());
  } else if (index == 2) {
    auto compIndex =
        static_cast<std::size_t>(ui->listCompartments->currentRow());
    ui->spinMaxTriangleArea->setValue(static_cast<int>(
        sbmlDoc.mesh.getCompartmentMaxTriangleArea(compIndex)));
    spinMaxTriangleArea_valueChanged(ui->spinMaxTriangleArea->value());
  }
}

void MainWindow::spinBoundaryIndex_valueChanged(int value) {
  const auto &size = ui->lblCompBoundary->size();
  auto boundaryIndex = static_cast<size_t>(value);
  ui->spinMaxBoundaryPoints->setValue(
      static_cast<int>(sbmlDoc.mesh.getBoundaryMaxPoints(boundaryIndex)));
  ui->lblCompBoundary->setPixmap(
      QPixmap::fromImage(sbmlDoc.mesh.getBoundariesImage(size, boundaryIndex)));
}

void MainWindow::spinMaxBoundaryPoints_valueChanged(int value) {
  const auto &size = ui->lblCompBoundary->size();
  auto boundaryIndex = static_cast<std::size_t>(ui->spinBoundaryIndex->value());
  sbmlDoc.mesh.setBoundaryMaxPoints(boundaryIndex, static_cast<size_t>(value));
  ui->lblCompBoundary->setPixmap(
      QPixmap::fromImage(sbmlDoc.mesh.getBoundariesImage(size, boundaryIndex)));
}

void MainWindow::spinMaxTriangleArea_valueChanged(int value) {
  const auto &size = ui->lblCompBoundary->size();
  auto compIndex = static_cast<std::size_t>(ui->listCompartments->currentRow());
  sbmlDoc.mesh.setCompartmentMaxTriangleArea(compIndex,
                                             static_cast<std::size_t>(value));
  ui->lblCompMesh->setPixmap(
      QPixmap::fromImage(sbmlDoc.mesh.getMeshImage(size, compIndex)));
}

void MainWindow::listCompartments_currentTextChanged(
    const QString &currentText) {
  ui->txtCompartmentSize->clear();
  if (!currentText.isEmpty()) {
    const QString &compID = currentText;
    SPDLOG_DEBUG("Compartment {} selected", compID.toStdString());
    ui->txtCompartmentSize->setText(
        QString::number(sbmlDoc.getCompartmentSize(compID)));
    QRgb col = sbmlDoc.getCompartmentColour(compID);
    SPDLOG_DEBUG("  - colour {:x} ", col);
    if (col == 0) {
      // null (transparent white) RGB colour: compartment does not have
      // an assigned colour in the image
      ui->lblCompShape->setPixmap(QPixmap());
      ui->lblCompartmentColour->setText("none");
      ui->lblCompShape->setPixmap(QPixmap());
      ui->lblCompShape->setText("none");
      ui->lblCompMesh->setPixmap(QPixmap());
      ui->lblCompMesh->setText("none");
    } else {
      // update colour box
      lblCompartmentColourPixmap.fill(QColor::fromRgb(col));
      ui->lblCompartmentColour->setPixmap(lblCompartmentColourPixmap);
      ui->lblCompartmentColour->setText("");
      // update image of compartment
      const auto &img =
          sbmlDoc.mapCompIdToGeometry.at(compID).getCompartmentImage();
      // rescale image
      if (img.width() * ui->lblCompShape->height() >
          img.height() * ui->lblCompShape->width()) {
        ui->lblCompShape->setPixmap(QPixmap::fromImage(img.scaledToWidth(
            ui->lblCompShape->width() - 2, Qt::FastTransformation)));
      } else {
        ui->lblCompShape->setPixmap(QPixmap::fromImage(img.scaledToHeight(
            ui->lblCompShape->height() - 2, Qt::FastTransformation)));
      }
      ui->lblCompShape->setText("");
      // update mesh or boundary image if tab is currently visible
      tabCompartmentGeometry_currentChanged(
          ui->tabCompartmentGeometry->currentIndex());
    }
  }
}

void MainWindow::listCompartments_itemDoubleClicked(QListWidgetItem *item) {
  // double-click on compartment list item is equivalent to
  // selecting item, then clicking on btnChangeCompartment
  if (item != nullptr) {
    btnChangeCompartment_clicked();
  }
}

void MainWindow::listMembranes_currentTextChanged(const QString &currentText) {
  if (!currentText.isEmpty()) {
    SPDLOG_DEBUG("Membrane {} selected", currentText.toStdString());
    // update image
    QPixmap pixmap = QPixmap::fromImage(sbmlDoc.getMembraneImage(currentText));
    ui->lblMembraneShape->setPixmap(pixmap);
  }
}

void MainWindow::listSpecies_currentItemChanged(QTreeWidgetItem *current,
                                                QTreeWidgetItem *previous) {
  Q_UNUSED(previous);
  // if user selects a species (i.e. an item with a parent)
  if ((current != nullptr) && (current->parent() != nullptr)) {
    QString speciesID = current->text(0);
    SPDLOG_DEBUG("Species {} selected", speciesID.toStdString());
    // display species information
    auto &field = sbmlDoc.mapSpeciesIdToField.at(speciesID);
    // spatial
    bool isSpatial = field.isSpatial;
    ui->chkSpeciesIsSpatial->setChecked(isSpatial);
    ui->txtDiffusionConstant->setEnabled(isSpatial);
    ui->radInitialConcentrationVarying->setEnabled(isSpatial);
    ui->btnImportConcentration->setEnabled(isSpatial);
    ui->cmbImportExampleConcentration->setEnabled(isSpatial);
    // constant
    bool isConstant = sbmlDoc.getIsSpeciesConstant(speciesID.toStdString());
    ui->chkSpeciesIsConstant->setChecked(isConstant);
    if (isConstant) {
      ui->txtDiffusionConstant->setEnabled(false);
    }
    // concentration
    ui->lblGeometryStatus->setText(
        QString("Species '%1' concentration:").arg(speciesID));
    ui->lblGeometry->setImage(sbmlDoc.getConcentrationImage(speciesID));
    if (field.isUniformConcentration) {
      ui->txtInitialConcentration->setText(
          QString::number(sbmlDoc.getInitialConcentration(speciesID)));
      ui->radInitialConcentrationUniform->setChecked(true);
    } else {
      ui->radInitialConcentrationVarying->setChecked(true);
    }
    // diffusion constant
    if (ui->txtDiffusionConstant->isEnabled()) {
      ui->txtDiffusionConstant->setText(
          QString::number(sbmlDoc.getDiffusionConstant(speciesID)));
    }
    // colour
    lblSpeciesColourPixmap.fill(sbmlDoc.getSpeciesColour(speciesID));
    ui->lblSpeciesColour->setPixmap(lblSpeciesColourPixmap);
    ui->lblSpeciesColour->setText("");
  }
}

void MainWindow::chkSpeciesIsSpatial_toggled(bool enabled) {
  const auto &speciesID = ui->listSpecies->currentItem()->text(0);
  // if new value differs from previous one - update model
  if (sbmlDoc.getIsSpatial(speciesID) != enabled) {
    SPDLOG_INFO("setting species {} isSpatial: {}", speciesID, enabled);
    sbmlDoc.setIsSpatial(speciesID, enabled);
    if (!enabled) {
      // must be spatially uniform if not spatial
      ui->radInitialConcentrationUniform->setChecked(enabled);
    }
    // disable incompatible options
    ui->txtDiffusionConstant->setEnabled(enabled);
    ui->radInitialConcentrationVarying->setEnabled(enabled);
    ui->btnImportConcentration->setEnabled(enabled);
    ui->cmbImportExampleConcentration->setEnabled(enabled);
    // update displayed info for this species
    txtInitialConcentration_editingFinished();
  }
}

void MainWindow::chkSpeciesIsConstant_toggled(bool enabled) {
  const auto &speciesID = ui->listSpecies->currentItem()->text(0).toStdString();
  // if new value differs from previous one - update model
  if (sbmlDoc.getIsSpeciesConstant(speciesID) != enabled) {
    SPDLOG_INFO("setting species {} isConstant: {}", speciesID, enabled);
    sbmlDoc.setIsSpeciesConstant(speciesID, enabled);
    // disable incompatible options
    ui->txtDiffusionConstant->setEnabled(!enabled);
    ui->radInitialConcentrationVarying->setEnabled(!enabled);
    ui->btnImportConcentration->setEnabled(!enabled);
    ui->cmbImportExampleConcentration->setEnabled(!enabled);
    // update displayed info for this species
    txtInitialConcentration_editingFinished();
  }
}

void MainWindow::radInitialConcentration_toggled() {
  if (ui->radInitialConcentrationUniform->isChecked()) {
    ui->txtInitialConcentration->setEnabled(true);
  } else {
    ui->txtInitialConcentration->setEnabled(false);
  }
}

void MainWindow::txtInitialConcentration_editingFinished() {
  double initConc = ui->txtInitialConcentration->text().toDouble();
  QString speciesID = ui->listSpecies->currentItem()->text(0);
  SPDLOG_INFO("setting initial concentration of Species {} to {}",
              speciesID.toStdString(), initConc);
  sbmlDoc.setInitialConcentration(speciesID, initConc);
  // update displayed info for this species
  listSpecies_currentItemChanged(ui->listSpecies->currentItem(), nullptr);
}

void MainWindow::btnAnalyticConcentration_clicked() {
  const auto &speciesID = ui->listSpecies->currentItem()->text(0);
  SPDLOG_DEBUG("ask user for analytic initial concentration of species {}...",
               speciesID);
  // todo: have dedicated window for this
  // which parses & checks expression is valid
  QString expr = QInputDialog::getText(
      this, "Analytic initial concentration",
      "Analytic initial concentration, e.g. \"cos(x)^2 + sin(y)^2:\"",
      QLineEdit::Normal, sbmlDoc.getAnalyticConcentration(speciesID));
  if (!expr.isEmpty()) {
    ui->radInitialConcentrationVarying->setChecked(true);
    SPDLOG_DEBUG("  - set expr: {}", expr);
    sbmlDoc.setAnalyticConcentration(speciesID, expr);
    ui->lblGeometry->setImage(sbmlDoc.getConcentrationImage(speciesID));
  }
}

void MainWindow::btnImportConcentration_clicked() {
  const auto &speciesID = ui->listSpecies->currentItem()->text(0);
  SPDLOG_DEBUG("ask user for concentration image for species {}... selected",
               speciesID);
  QString filename = QFileDialog::getOpenFileName(
      this, "Import species concentration from image", "",
      "Image Files (*.png *.jpg *.bmp *.tiff)", nullptr,
      QFileDialog::Option::DontUseNativeDialog);
  if (!filename.isEmpty()) {
    ui->radInitialConcentrationVarying->setChecked(true);
    SPDLOG_DEBUG("  - import file {}", filename);
    sbmlDoc.importConcentrationFromImage(speciesID, filename);
    ui->lblGeometry->setImage(sbmlDoc.getConcentrationImage(speciesID));
  }
}

void MainWindow::cmbImportExampleConcentration_currentTextChanged(
    const QString &text) {
  if (ui->cmbImportExampleConcentration->currentIndex() != 0) {
    const auto &speciesID = ui->listSpecies->currentItem()->text(0);
    ui->radInitialConcentrationVarying->setChecked(true);
    SPDLOG_DEBUG("import {}", text);
    sbmlDoc.importConcentrationFromImage(
        speciesID, QString(":/concentration/%1.png").arg(text));
    ui->lblGeometry->setImage(sbmlDoc.getConcentrationImage(speciesID));
    ui->cmbImportExampleConcentration->setCurrentIndex(0);
  }
}

void MainWindow::txtDiffusionConstant_editingFinished() {
  double diffConst = ui->txtDiffusionConstant->text().toDouble();
  QString speciesID = ui->listSpecies->currentItem()->text(0);
  SPDLOG_INFO("setting Diffusion Constant of Species {} to {}",
              speciesID.toStdString(), diffConst);
  sbmlDoc.setDiffusionConstant(speciesID, diffConst);
}

void MainWindow::btnChangeSpeciesColour_clicked() {
  QString speciesID = ui->listSpecies->currentItem()->text(0);
  SPDLOG_DEBUG("waiting for new colour for species {} from user...",
               speciesID.toStdString());
  QColor newCol = QColorDialog::getColor(sbmlDoc.getSpeciesColour(speciesID),
                                         this, "Choose new species colour",
                                         QColorDialog::DontUseNativeDialog);
  if (newCol.isValid()) {
    SPDLOG_DEBUG("  - set new colour to {:x}", newCol.rgba());
    sbmlDoc.setSpeciesColour(speciesID, newCol);
    listSpecies_currentItemChanged(ui->listSpecies->currentItem(), nullptr);
  }
}

void MainWindow::listReactions_currentItemChanged(QTreeWidgetItem *current,
                                                  QTreeWidgetItem *previous) {
  Q_UNUSED(previous);
  ui->listProducts->clear();
  ui->listReactants->clear();
  ui->listReactionParams->clear();
  ui->lblReactionRate->clear();
  // if user selects a species (i.e. an item with a parent)
  if ((current != nullptr) && (current->parent() != nullptr)) {
    const QString &compID = current->parent()->text(0);
    const QString &reacID = current->text(0);
    SPDLOG_DEBUG("Reaction {} selected", reacID.toStdString());
    // display image of reaction compartment or membrane
    if (std::find(sbmlDoc.compartments.cbegin(), sbmlDoc.compartments.cend(),
                  compID) != sbmlDoc.compartments.cend()) {
      ui->lblGeometry->setImage(
          sbmlDoc.mapCompIdToGeometry.at(compID).getCompartmentImage());
    } else {
      ui->lblGeometry->setImage(sbmlDoc.getMembraneImage(compID));
    }
    // display reaction information
    const auto *reac = sbmlDoc.getReaction(reacID);
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

void MainWindow::listFunctions_currentTextChanged(const QString &currentText) {
  ui->listFunctionParams->clear();
  ui->lblFunctionDef->clear();
  if (currentText.size() > 0) {
    SPDLOG_DEBUG("Function {} selected", currentText.toStdString());
    const auto *func = sbmlDoc.getFunctionDefinition(currentText);
    for (unsigned i = 0; i < func->getNumArguments(); ++i) {
      ui->listFunctionParams->addItem(
          libsbml::SBML_formulaToL3String(func->getArgument(i)));
      // todo: check the above is not leaking a malloced char *
    }
    ui->lblFunctionDef->setText(
        libsbml::SBML_formulaToL3String(func->getBody()));
    // todo: check the above is not leaking a malloced char *
  }
}

void MainWindow::btnSimulate_clicked() {
  // simple 2d spatial simulation
  simulate::Simulate sim(&sbmlDoc, simMathBackend);
  // add compartments
  for (const auto &compartmentID : sbmlDoc.compartments) {
    sim.addCompartment(&sbmlDoc.mapCompIdToGeometry.at(compartmentID));
  }
  // add membranes
  for (auto &membrane : sbmlDoc.membraneVec) {
    sim.addMembrane(&membrane);
  }

  // Dune simulation
  dune::DuneSimulation duneSim(sbmlDoc, ui->lblGeometry->size());

  // get initial concentrations
  images.clear();
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

  ui->statusBar->showMessage("Simulating...");
  QTime qtime;
  qtime.start();
  isSimulationRunning = true;
  this->setCursor(Qt::WaitCursor);
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
    ui->lblGeometry->setImage(images.back());
    ui->statusBar->showMessage(
        QString("Simulating... %1% (press ctrl+c to cancel)")
            .arg(QString::number(static_cast<int>(
                100 * t / ui->txtSimLength->text().toDouble()))));
  }

  // plot results
  ui->pltPlot->clearGraphs();
  ui->pltPlot->setInteraction(QCP::iRangeDrag, true);
  ui->pltPlot->setInteraction(QCP::iRangeZoom, true);
  ui->pltPlot->setInteraction(QCP::iSelectPlottables, true);
  ui->pltPlot->legend->setVisible(true);
  for (std::size_t i = 0; i < sim.speciesID.size(); ++i) {
    auto *graph = ui->pltPlot->addGraph();
    graph->setData(time, conc[i]);
    graph->setPen(sim.field[i]->colour);
    graph->setName(sim.speciesID[i].c_str());
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

  // populate species display list
  QMenu *menu = new QMenu(this);
  for (const auto &s : sim.speciesID) {
    QAction *act = new QAction(s.c_str(), menu);
    act->setCheckable(true);
    act->setChecked(true);
    menu->addAction(act);
    connect(act, &QAction::triggered, this,
            &MainWindow::updateSpeciesDisplaySelect);
  }
  ui->btnSpeciesDisplaySelect->setMenu(menu);

  ui->statusBar->showMessage("Simulation complete.");
  SPDLOG_INFO("simulation run-time: {}", qtime.elapsed());
  this->setCursor(Qt::ArrowCursor);
}

void MainWindow::btnResetSimulation_clicked() {
  ui->pltPlot->clearGraphs();
  ui->pltPlot->replot();
  ui->hslideTime->setMinimum(0);
  ui->hslideTime->setMaximum(0);
  ui->hslideTime->setEnabled(false);
  images.clear();
  // reset all fields to their initial values
  for (auto &field : sbmlDoc.mapSpeciesIdToField) {
    field.second.conc = field.second.init;
  }
  tabMain_updateSimulate();
}

void MainWindow::graphClicked(QCPAbstractPlottable *plottable, int dataIndex) {
  double dataValue = plottable->interface1D()->dataMainValue(dataIndex);
  QString message =
      QString("Clicked on graph '%1' at data point #%2 with value %3.")
          .arg(plottable->name())
          .arg(dataIndex)
          .arg(dataValue);
  qDebug() << message;
  ui->hslideTime->setValue(dataIndex);
}

void MainWindow::hslideTime_valueChanged(int value) {
  if (images.size() > value) {
    ui->lblGeometry->setImage(images[value]);
  }
}
