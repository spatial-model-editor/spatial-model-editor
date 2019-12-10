#include "mainwindow.hpp"

#include <qcustomplot.h>

#include <QErrorMessage>
#include <QFileDialog>
#include <QInputDialog>
#include <QMessageBox>
#include <QString>
#include <QStringListModel>
#include <sstream>

#include "dialogabout.hpp"
#include "dialoganalytic.hpp"
#include "dialogconcentrationimage.hpp"
#include "dialogimagesize.hpp"
#include "dialogunits.hpp"
#include "dune.hpp"
#include "logger.hpp"
#include "mesh.hpp"
#include "sbml.hpp"
#include "simulate.hpp"
#include "tiff.hpp"
#include "ui_mainwindow.h"
#include "utils.hpp"
#include "version.hpp"

static void selectFirstChild(QTreeWidget *tree) {
  for (int i = 0; i < tree->topLevelItemCount(); ++i) {
    if (auto *p = tree->topLevelItem(i);
        (p != nullptr) && (p->childCount() > 0)) {
      tree->setCurrentItem(p->child(0));
      return;
    }
  }
}

static void selectMatchingOrFirstItem(QListWidget *list,
                                      const QString &text = {}) {
  if (list->count() == 0) {
    return;
  }
  if (auto l = list->findItems(text, Qt::MatchExactly); !l.isEmpty()) {
    list->setCurrentItem(l[0]);
  } else {
    list->setCurrentRow(0);
  }
}

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent), ui(new Ui::MainWindow) {
  ui->setupUi(this);

  Q_INIT_RESOURCE(resources);

  shortcutStopSimulation = new QShortcut(this);
  shortcutStopSimulation->setKey(Qt::CTRL + Qt::Key_C);

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

  connect(ui->actionSet_model_units, &QAction::triggered, this,
          &MainWindow::actionSet_model_units_triggered);

  connect(ui->actionSet_image_size, &QAction::triggered, this,
          &MainWindow::actionSet_image_size_triggered);

  connect(ui->action_What_s_this, &QAction::triggered, this,
          []() { QWhatsThis::enterWhatsThisMode(); });

  connect(ui->actionOnline_Documentation, &QAction::triggered, this, []() {
    QDesktopServices::openUrl(
        QUrl(QString("https://spatial-model-editor.readthedocs.io")));
  });

  connect(ui->action_About, &QAction::triggered, this,
          [this]() { DialogAbout(this).exec(); });

  connect(ui->actionAbout_Qt, &QAction::triggered, this,
          [this]() { QMessageBox::aboutQt(this); });
  // geometry
  connect(ui->lblGeometry, &QLabelMouseTracker::mouseClicked, this,
          &MainWindow::lblGeometry_mouseClicked);

  connect(ui->btnChangeCompartment, &QPushButton::clicked, this,
          &MainWindow::btnChangeCompartment_clicked);

  connect(ui->btnSetCompartmentSizeFromImage, &QPushButton::clicked, this,
          &MainWindow::btnSetCompartmentSizeFromImage_clicked);

  connect(ui->tabCompartmentGeometry, &QTabWidget::currentChanged, this,
          &MainWindow::tabCompartmentGeometry_currentChanged);

  connect(ui->lblCompBoundary, &QLabelMouseTracker::mouseClicked, this,
          &MainWindow::lblCompBoundary_mouseClicked);

  connect(ui->lblCompBoundary, &QLabelMouseTracker::mouseWheelEvent, this,
          [this](QWheelEvent *ev) {
            QApplication::sendEvent(ui->spinMaxBoundaryPoints, ev);
          });

  connect(ui->spinBoundaryIndex, qOverload<int>(&QSpinBox::valueChanged), this,
          &MainWindow::spinBoundaryIndex_valueChanged);

  connect(ui->spinMaxBoundaryPoints, qOverload<int>(&QSpinBox::valueChanged),
          this, &MainWindow::spinMaxBoundaryPoints_valueChanged);

  connect(ui->spinBoundaryWidth,
          qOverload<double>(&QDoubleSpinBox::valueChanged), this,
          &MainWindow::spinBoundaryWidth_valueChanged);

  connect(ui->lblCompMesh, &QLabelMouseTracker::mouseClicked, this,
          &MainWindow::lblCompMesh_mouseClicked);

  connect(ui->lblCompMesh, &QLabelMouseTracker::mouseWheelEvent, this,
          [this](QWheelEvent *ev) {
            QApplication::sendEvent(ui->spinMaxTriangleArea, ev);
          });

  connect(ui->spinMaxTriangleArea, qOverload<int>(&QSpinBox::valueChanged),
          this, &MainWindow::spinMaxTriangleArea_valueChanged);

  connect(ui->listCompartments, &QListWidget::currentRowChanged, this,
          &MainWindow::listCompartments_currentRowChanged);

  connect(ui->listCompartments, &QListWidget::itemDoubleClicked, this,
          &MainWindow::listCompartments_itemDoubleClicked);

  // membranes
  connect(ui->listMembranes, &QListWidget::currentRowChanged, this,
          &MainWindow::listMembranes_currentRowChanged);

  // species
  connect(ui->listSpecies, &QTreeWidget::currentItemChanged, this,
          &MainWindow::listSpecies_currentItemChanged);

  connect(ui->btnAddSpecies, &QPushButton::clicked, this,
          &MainWindow::btnAddSpecies_clicked);

  connect(ui->btnRemoveSpecies, &QPushButton::clicked, this,
          &MainWindow::btnRemoveSpecies_clicked);

  connect(ui->txtSpeciesName, &QLineEdit::editingFinished, this,
          &MainWindow::txtSpeciesName_editingFinished);

  connect(ui->cmbSpeciesCompartment, qOverload<int>(&QComboBox::activated),
          this, &MainWindow::cmbSpeciesCompartment_activated);

  connect(ui->chkSpeciesIsSpatial, &QCheckBox::toggled, this,
          &MainWindow::chkSpeciesIsSpatial_toggled);

  connect(ui->chkSpeciesIsConstant, &QCheckBox::toggled, this,
          &MainWindow::chkSpeciesIsConstant_toggled);

  connect(ui->radInitialConcentrationUniform, &QRadioButton::toggled, this,
          &MainWindow::radInitialConcentration_toggled);

  connect(ui->txtInitialConcentration, &QLineEdit::editingFinished, this,
          &MainWindow::txtInitialConcentration_editingFinished);

  connect(ui->radInitialConcentrationImage, &QRadioButton::toggled, this,
          &MainWindow::radInitialConcentration_toggled);

  connect(ui->radInitialConcentrationAnalytic, &QRadioButton::toggled, this,
          &MainWindow::radInitialConcentration_toggled);

  connect(ui->btnEditAnalyticConcentration, &QPushButton::clicked, this,
          &MainWindow::btnEditAnalyticConcentration_clicked);

  connect(ui->btnEditImageConcentration, &QPushButton::clicked, this,
          &MainWindow::btnEditImageConcentration_clicked);

  connect(ui->txtDiffusionConstant, &QLineEdit::editingFinished, this,
          &MainWindow::txtDiffusionConstant_editingFinished);

  connect(ui->btnChangeSpeciesColour, &QPushButton::clicked, this,
          &MainWindow::btnChangeSpeciesColour_clicked);

  // reactions
  connect(ui->listReactions, &QTreeWidget::currentItemChanged, this,
          &MainWindow::listReactions_currentItemChanged);

  connect(ui->btnAddReaction, &QPushButton::clicked, this,
          &MainWindow::btnAddReaction_clicked);

  connect(ui->btnRemoveReaction, &QPushButton::clicked, this,
          &MainWindow::btnRemoveReaction_clicked);

  connect(ui->cmbReactionLocation, qOverload<int>(&QComboBox::activated), this,
          &MainWindow::cmbReactionLocation_activated);

  connect(ui->listReactionParams, &QTableWidget::currentCellChanged, this,
          &MainWindow::listReactionParams_currentCellChanged);

  connect(ui->btnAddReactionParam, &QPushButton::clicked, this,
          &MainWindow::btnAddReactionParam_clicked);

  connect(ui->btnRemoveReactionParam, &QPushButton::clicked, this,
          &MainWindow::btnRemoveReactionParam_clicked);

  connect(ui->txtReactionRate, &QPlainTextMathEdit::mathChanged, this,
          &MainWindow::txtReactionRate_mathChanged);

  connect(ui->btnSaveReactionChanges, &QPushButton::clicked, this,
          &MainWindow::btnSaveReactionChanges_clicked);

  // functions
  connect(ui->listFunctions, &QListWidget::currentRowChanged, this,
          &MainWindow::listFunctions_currentRowChanged);

  connect(ui->btnAddFunction, &QPushButton::clicked, this,
          &MainWindow::btnAddFunction_clicked);

  connect(ui->btnRemoveFunction, &QPushButton::clicked, this,
          &MainWindow::btnRemoveFunction_clicked);

  connect(ui->listFunctionParams, &QListWidget::currentRowChanged, this,
          &MainWindow::listFunctionParams_currentRowChanged);

  connect(ui->btnAddFunctionParam, &QPushButton::clicked, this,
          &MainWindow::btnAddFunctionParam_clicked);

  connect(ui->btnRemoveFunctionParam, &QPushButton::clicked, this,
          &MainWindow::btnRemoveFunctionParam_clicked);

  connect(ui->txtFunctionDef, &QPlainTextMathEdit::mathChanged, this,
          &MainWindow::txtFunctionDef_mathChanged);

  connect(ui->btnSaveFunctionChanges, &QPushButton::clicked, this,
          &MainWindow::btnSaveFunctionChanges_clicked);

  // simulate
  connect(ui->actionGroupSimType, &QActionGroup::triggered, this,
          [this](QAction *action) {
            Q_UNUSED(action);
            useDuneSimulator = ui->actionSimTypeDUNE->isChecked();
          });

  connect(ui->btnSimulate, &QPushButton::clicked, this,
          &MainWindow::btnSimulate_clicked);

  connect(ui->btnResetSimulation, &QPushButton::clicked, this,
          &MainWindow::btnResetSimulation_clicked);

  connect(shortcutStopSimulation, &QShortcut::activated, this,
          [this]() { isSimulationRunning = false; });

  connect(ui->pltPlot, &QCustomPlot::plottableClick, this,
          &MainWindow::graphClicked);

  connect(ui->hslideTime, &QSlider::valueChanged, this,
          &MainWindow::hslideTime_valueChanged);
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
      ui->txtGMSH->setText(sbmlDoc.mesh->getGMSH());
      break;
    default:
      qFatal("ui::tabMain :: Errror: Tab index %d not valid", index);
  }
}

void MainWindow::tabMain_updateGeometry() {
  ui->listCompartments->clear();
  ui->listCompartments->insertItems(0, sbmlDoc.compartmentNames);
  if (ui->listCompartments->count() > 0) {
    ui->listCompartments->setCurrentRow(0);
  }
  ui->lblGeometry->setImage(sbmlDoc.getCompartmentImage());
  ui->lblGeometryStatus->setText("Compartment Geometry:");
}

void MainWindow::tabMain_updateMembranes() {
  ui->lblMembraneShape->clear();
  ui->listMembranes->clear();
  ui->listMembranes->addItems(sbmlDoc.membraneNames);
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
  ui->cmbSpeciesCompartment->clear();
  for (int iComp = 0; iComp < sbmlDoc.compartments.size(); ++iComp) {
    // add compartments as top level items
    QString compName = sbmlDoc.compartmentNames[iComp];
    QTreeWidgetItem *comp = new QTreeWidgetItem(ls, {compName});
    // also add to species compartment combo box
    ui->cmbSpeciesCompartment->addItem(compName);
    ls->addTopLevelItem(comp);
    for (const auto &s : sbmlDoc.species.at(sbmlDoc.compartments[iComp])) {
      // add each species as child of compartment
      comp->addChild(
          new QTreeWidgetItem(comp, QStringList({sbmlDoc.getSpeciesName(s)})));
    }
  }
  ls->expandAll();
  selectFirstChild(ls);
}

void MainWindow::tabMain_updateReactions() {
  ui->lblGeometryStatus->setText("Reaction location:");
  ui->cmbReactionLocation->clear();
  ui->listReactionParams->clear();
  auto *ls = ui->listReactions;
  ls->clear();
  for (int i = 0; i < sbmlDoc.compartments.size(); ++i) {
    // add compartment as top level item
    const auto &name = sbmlDoc.compartmentNames.at(i);
    QTreeWidgetItem *comp = new QTreeWidgetItem(ls, QStringList({name}));
    ls->addTopLevelItem(comp);
    ui->cmbReactionLocation->addItem(name);
    for (const auto &reacID : sbmlDoc.reactions.at(sbmlDoc.compartments[i])) {
      // add each reaction as child of compartment
      comp->addChild(new QTreeWidgetItem(
          comp, QStringList({sbmlDoc.getReactionName(reacID)})));
    }
  }
  for (int i = 0; i < sbmlDoc.membranes.size(); ++i) {
    // add compartment as top level item
    const auto &name = sbmlDoc.membraneNames.at(i);
    QTreeWidgetItem *memb = new QTreeWidgetItem(ls, QStringList({name}));
    ls->addTopLevelItem(memb);
    ui->cmbReactionLocation->addItem(name);
    for (const auto &reacID : sbmlDoc.reactions.at(sbmlDoc.membranes[i])) {
      // add each reaction as child of compartment
      memb->addChild(new QTreeWidgetItem(
          memb, QStringList({sbmlDoc.getReactionName(reacID)})));
    }
  }
  ls->expandAll();
  selectFirstChild(ls);
}

void MainWindow::tabMain_updateFunctions(const QString &selection) {
  auto *list = ui->listFunctions;
  list->clear();
  ui->btnRemoveFunctionParam->setEnabled(false);
  for (const auto &funcID : sbmlDoc.functions) {
    list->addItem(sbmlDoc.getFunctionDefinition(funcID).name.c_str());
  }
  selectMatchingOrFirstItem(list, selection);
  bool enable = list->count() > 0;
  ui->txtFunctionName->setEnabled(enable);
  ui->listFunctionParams->setEnabled(enable);
  ui->txtFunctionDef->setEnabled(enable);
}

void MainWindow::tabMain_updateSimulate() {
  ui->lblGeometryStatus->setText("Simulation concentration:");
  ui->hslideTime->setVisible(true);
  ui->hslideTime->setEnabled(true);
  ui->hslideTime->setValue(0);
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
  bool enable = sbmlDoc.isValid && sbmlDoc.hasValidGeometry;
  for (int i = 1; i < ui->tabMain->count(); ++i) {
    ui->tabMain->setTabEnabled(i, enable);
  }
  ui->tabCompartmentGeometry->setTabEnabled(1, enable);
  ui->tabCompartmentGeometry->setTabEnabled(2, enable);
}

void MainWindow::action_Open_SBML_file_triggered() {
  QString filename = QFileDialog::getOpenFileName(
      this, "Open SBML file", "", "SBML file (*.xml);; All files (*.*)",
      nullptr, QFileDialog::Option::DontUseNativeDialog);
  if (!filename.isEmpty()) {
    sbmlDoc = sbml::SbmlDocWrapper();
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
    SPDLOG_WARN("failed to open built-in file: {}", filename.toStdString());
    return;
  }
  sbmlDoc = sbml::SbmlDocWrapper();
  sbmlDoc.importSBMLString(f.readAll().toStdString());
  ui->tabMain->setCurrentIndex(0);
  tabMain_currentChanged(0);
  enableTabs();
  images.clear();
}

void MainWindow::action_Save_SBML_file_triggered() {
  if (!isValidModelAndGeometryImage()) {
    SPDLOG_DEBUG("invalid geometry and/or model: ignoring");
    return;
  }
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
  if (!isValidModelAndGeometryImage()) {
    SPDLOG_DEBUG("invalid geometry and/or model: ignoring");
    return;
  }
  QString filename = QFileDialog::getSaveFileName(
      this, "Export DUNE ini file", "", "DUNE ini file (*.ini)", nullptr,
      QFileDialog::Option::DontUseNativeDialog);
  if (!filename.isEmpty()) {
    if (filename.right(4) != ".ini") {
      filename.append(".ini");
    }
    dune::DuneConverter dc(sbmlDoc);
    QFile f(filename);
    if (f.open(QIODevice::ReadWrite | QIODevice::Text)) {
      f.write(dc.getIniFile().toUtf8());
    }
    // also export gmsh file `grid.msh` in the same dir
    QString dir = QFileInfo(filename).absolutePath();
    QString meshFilename = QDir(dir).filePath("grid.msh");
    QFile f2(meshFilename);
    if (f2.open(QIODevice::ReadWrite | QIODevice::Text)) {
      f2.write(sbmlDoc.mesh->getGMSH(dc.getGMSHCompIndices()).toUtf8());
    }
  }
}

void MainWindow::actionGeometry_from_image_triggered() {
  if (!isValidModel()) {
    return;
  }
  QString filename = QFileDialog::getOpenFileName(
      this, "Import geometry from image", "",
      "Image Files (*.tif *.tiff *.gif *.jpg *.jpeg *.png *.bmp);; All files "
      "(*.*)",
      nullptr, QFileDialog::Option::DontUseNativeDialog);
  if (!filename.isEmpty()) {
    utils::TiffReader tiffReader(filename.toStdString());
    QImage img;
    if (tiffReader.size() == 0) {
      img.load(filename);
    } else if (tiffReader.size() == 1) {
      img = tiffReader.getImage();
    } else {
      bool ok;
      int i = QInputDialog::getInt(
          this, "Import tiff image",
          "Please choose the page to use from this multi-page tiff", 0, 0,
          static_cast<int>(tiffReader.size()) - 1, 1, &ok);
      if (ok) {
        img = tiffReader.getImage(static_cast<std::size_t>(i));
      } else {
        return;
      }
    }
    importGeometryImage(img);
  }
}

void MainWindow::menuExample_geometry_image_triggered(QAction *action) {
  if (!isValidModel()) {
    return;
  }
  QString filename =
      QString(":/geometry/%1.png").arg(action->text().remove(0, 1));
  QImage img(filename);
  importGeometryImage(img);
}

void MainWindow::importGeometryImage(const QImage &image) {
  sbmlDoc.importGeometryFromImage(image);
  ui->tabMain->setCurrentIndex(0);
  tabMain_currentChanged(0);
  enableTabs();
  images.clear();
  // set default pixelwidth in case user doesn't set image physical size
  sbmlDoc.setPixelWidth(1.0);
  actionSet_image_size_triggered();
}

void MainWindow::actionSet_model_units_triggered() {
  if (!isValidModel()) {
    return;
  }
  units::Unit oldLengthUnit = sbmlDoc.getModelUnits().getLength();
  double oldPixelWidth = sbmlDoc.getPixelWidth();
  DialogUnits dialog(sbmlDoc.getModelUnits());
  if (dialog.exec() == QDialog::Accepted) {
    sbmlDoc.setUnitsTimeIndex(dialog.getTimeUnitIndex());
    sbmlDoc.setUnitsLengthIndex(dialog.getLengthUnitIndex());
    sbmlDoc.setUnitsVolumeIndex(dialog.getVolumeUnitIndex());
    sbmlDoc.setUnitsAmountIndex(dialog.getAmountUnitIndex());
    // rescale pixelsize to match new units
    sbmlDoc.setPixelWidth(units::rescale(oldPixelWidth, oldLengthUnit,
                                         sbmlDoc.getModelUnits().getLength()));
  }
  return;
}

void MainWindow::actionSet_image_size_triggered() {
  if (!isValidModelAndGeometryImage()) {
    return;
  }
  DialogImageSize dialog(sbmlDoc.getCompartmentImage(), sbmlDoc.getPixelWidth(),
                         sbmlDoc.getModelUnits());
  if (dialog.exec() == QDialog::Accepted) {
    double pixelWidth = dialog.getPixelWidth();
    SPDLOG_INFO("Set new pixel width = {}", pixelWidth);
    sbmlDoc.setPixelWidth(pixelWidth);
  }
}

void MainWindow::lblGeometry_mouseClicked(QRgb col, QPoint point) {
  if (waitingForCompartmentChoice) {
    // update compartment geometry (i.e. colour) of selected compartment to
    // the one the user just clicked on
    const auto &compartmentID =
        sbmlDoc.compartments.at(ui->listCompartments->currentRow());
    sbmlDoc.setCompartmentColour(compartmentID, col);
    sbmlDoc.setCompartmentInteriorPoint(compartmentID, point);
    ui->tabCompartmentGeometry->setCurrentIndex(0);
    // update display by simulating user click on listCompartments
    listCompartments_currentRowChanged(ui->listCompartments->currentRow());
    enableTabs();
    SPDLOG_INFO("assigned compartment {} to colour {:x}",
                compartmentID.toStdString(), col);
    waitingForCompartmentChoice = false;
    statusBarPermanentMessage->clear();
  } else {
    // display compartment the user just clicked on
    auto compID = sbmlDoc.getCompartmentID(col);
    for (int i = 0; i < sbmlDoc.compartments.size(); ++i) {
      if (sbmlDoc.compartments.at(i) == compID) {
        ui->listCompartments->setCurrentRow(i);
      }
    }
  }
}

bool MainWindow::isValidModel() {
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
  return true;
}

bool MainWindow::isValidModelAndGeometryImage() {
  if (!isValidModel()) {
    return false;
  }
  if (sbmlDoc.hasGeometryImage == false) {
    SPDLOG_DEBUG("  - no geometry image");
    if (QMessageBox::question(this, "No compartment geometry image",
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
  if (!isValidModelAndGeometryImage()) {
    SPDLOG_DEBUG("invalid geometry and/or model: ignoring");
    return;
  }
  SPDLOG_DEBUG("waiting for user to click on geometry image..");
  waitingForCompartmentChoice = true;
  statusBarPermanentMessage->setText(
      "Please click on the desired location on the compartment geometry "
      "image...");
}

void MainWindow::btnSetCompartmentSizeFromImage_clicked() {
  const auto &compartmentID =
      sbmlDoc.compartments.at(ui->listCompartments->currentRow());
  sbmlDoc.setCompartmentSizeFromImage(compartmentID.toStdString());
  listCompartments_currentRowChanged(ui->listCompartments->currentRow());
}

void MainWindow::tabCompartmentGeometry_currentChanged(int index) {
  enum TabIndex { IMAGE = 0, BOUNDARIES = 1, MESH = 2 };
  SPDLOG_DEBUG("Tab changed to {} [{}]", index,
               ui->tabCompartmentGeometry->tabText(index).toStdString());
  if (index == TabIndex::BOUNDARIES) {
    auto size = sbmlDoc.mesh->getBoundaries().size();
    ui->spinBoundaryIndex->setMaximum(static_cast<int>(size) - 1);
    spinBoundaryIndex_valueChanged(ui->spinBoundaryIndex->value());
  } else if (index == TabIndex::MESH) {
    auto compIndex =
        static_cast<std::size_t>(ui->listCompartments->currentRow());
    ui->spinMaxTriangleArea->setValue(static_cast<int>(
        sbmlDoc.mesh->getCompartmentMaxTriangleArea(compIndex)));
    spinMaxTriangleArea_valueChanged(ui->spinMaxTriangleArea->value());
  }
}

void MainWindow::lblCompBoundary_mouseClicked(QRgb col, QPoint point) {
  Q_UNUSED(col);
  Q_UNUSED(point);
  auto index = ui->lblCompBoundary->getMaskIndex();
  if (index <= ui->spinBoundaryIndex->maximum() &&
      index != ui->spinBoundaryIndex->value()) {
    ui->spinBoundaryIndex->setValue(index);
  }
}

void MainWindow::spinBoundaryIndex_valueChanged(int value) {
  const auto &size = ui->lblCompBoundary->size();
  auto boundaryIndex = static_cast<size_t>(value);
  ui->spinMaxBoundaryPoints->setValue(
      static_cast<int>(sbmlDoc.mesh->getBoundaryMaxPoints(boundaryIndex)));
  ui->lblCompBoundary->setImages(
      sbmlDoc.mesh->getBoundariesImages(size, boundaryIndex));
  if (sbmlDoc.mesh->isMembrane(boundaryIndex)) {
    ui->spinBoundaryWidth->setEnabled(true);
    ui->spinBoundaryWidth->setValue(
        sbmlDoc.mesh->getBoundaryWidth(boundaryIndex));
  } else {
    ui->spinBoundaryWidth->setEnabled(false);
  }
}

void MainWindow::spinMaxBoundaryPoints_valueChanged(int value) {
  const auto &size = ui->lblCompBoundary->size();
  auto boundaryIndex = static_cast<std::size_t>(ui->spinBoundaryIndex->value());
  sbmlDoc.mesh->setBoundaryMaxPoints(boundaryIndex, static_cast<size_t>(value));
  ui->lblCompBoundary->setImages(
      sbmlDoc.mesh->getBoundariesImages(size, boundaryIndex));
}

void MainWindow::spinBoundaryWidth_valueChanged(double value) {
  const auto &size = ui->lblCompBoundary->size();
  auto boundaryIndex = static_cast<std::size_t>(ui->spinBoundaryIndex->value());
  sbmlDoc.mesh->setBoundaryWidth(boundaryIndex, value);
  ui->lblCompBoundary->setImages(
      sbmlDoc.mesh->getBoundariesImages(size, boundaryIndex));
}

void MainWindow::lblCompMesh_mouseClicked(QRgb col, QPoint point) {
  Q_UNUSED(col);
  Q_UNUSED(point);
  auto index = ui->lblCompMesh->getMaskIndex();
  if (index < ui->listCompartments->count() &&
      index != ui->listCompartments->currentRow()) {
    ui->listCompartments->setCurrentRow(index);
    ui->spinMaxTriangleArea->setFocus();
    ui->spinMaxTriangleArea->selectAll();
  }
}

void MainWindow::spinMaxTriangleArea_valueChanged(int value) {
  const auto &size = ui->lblCompMesh->size();
  auto compIndex = static_cast<std::size_t>(ui->listCompartments->currentRow());
  sbmlDoc.mesh->setCompartmentMaxTriangleArea(compIndex,
                                              static_cast<std::size_t>(value));
  ui->lblCompMesh->setImages(sbmlDoc.mesh->getMeshImages(size, compIndex));
}

void MainWindow::listCompartments_currentRowChanged(int currentRow) {
  ui->txtCompartmentSize->clear();
  if (currentRow >= 0 && currentRow < ui->listCompartments->count()) {
    const QString &compID = sbmlDoc.compartments.at(currentRow);
    SPDLOG_DEBUG("row {} selected", currentRow);
    SPDLOG_DEBUG("  - Compartment Name: {}",
                 ui->listCompartments->currentItem()->text().toStdString());
    SPDLOG_DEBUG("  - Compartment Id: {}", compID.toStdString());
    ui->txtCompartmentSize->setText(
        QString::number(sbmlDoc.getCompartmentSize(compID)));
    ui->lblCompartmentSizeUnits->setText(
        sbmlDoc.getModelUnits().getVolume().symbol);
    QRgb col = sbmlDoc.getCompartmentColour(compID);
    SPDLOG_DEBUG("  - Compartment colour {:x} ", col);
    if (col == 0) {
      // null (transparent white) RGB colour: compartment does not have
      // an assigned colour in the image
      ui->btnSetCompartmentSizeFromImage->setEnabled(false);
      ui->lblCompShape->setImage(QImage());
      ui->lblCompartmentColour->setText("none");
      ui->lblCompShape->setImage(QImage());
      ui->lblCompShape->setText(
          "<p>Compartment has no assigned geometry</p> "
          "<ul><li>please click on the 'Select compartment geometry...' "
          "button below</li> "
          "<li> then on the desired location in the geometry "
          "image on the left</li></ul>");
      ui->lblCompMesh->setImage(QImage());
      ui->lblCompMesh->setText("none");
    } else {
      ui->btnSetCompartmentSizeFromImage->setEnabled(true);
      // update colour box
      lblCompartmentColourPixmap.fill(QColor::fromRgb(col));
      ui->lblCompartmentColour->setPixmap(lblCompartmentColourPixmap);
      ui->lblCompartmentColour->setText("");
      // update image of compartment
      ui->lblCompShape->setImage(
          sbmlDoc.mapCompIdToGeometry.at(compID).getCompartmentImage());
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

void MainWindow::listMembranes_currentRowChanged(int currentRow) {
  if (currentRow >= 0 && currentRow < ui->listMembranes->count()) {
    const QString &membraneID = sbmlDoc.membranes.at(currentRow);
    SPDLOG_DEBUG("row {} selected", currentRow);
    SPDLOG_DEBUG("  - Membrane Name: {}",
                 ui->listMembranes->currentItem()->text().toStdString());
    SPDLOG_DEBUG("  - Membrane Id: {}", membraneID.toStdString());
    // update image
    QPixmap pixmap = QPixmap::fromImage(sbmlDoc.getMembraneImage(membraneID));
    ui->lblMembraneShape->setPixmap(pixmap);
  }
}

void MainWindow::listSpecies_currentItemChanged(QTreeWidgetItem *current,
                                                QTreeWidgetItem *previous) {
  Q_UNUSED(previous);
  // if user selects a species (i.e. an item with a parent)
  if ((current != nullptr) && (current->parent() != nullptr)) {
    SPDLOG_DEBUG("item {} / {} selected",
                 current->parent()->text(0).toStdString(),
                 current->text(0).toStdString());
    int compartmentIndex =
        ui->listSpecies->indexOfTopLevelItem(current->parent());
    QString compartmentID = sbmlDoc.compartments.at(compartmentIndex);
    int speciesIndex = current->parent()->indexOfChild(current);
    QString speciesID = sbmlDoc.species.at(compartmentID).at(speciesIndex);
    sbmlDoc.currentSpecies = speciesID;
    SPDLOG_DEBUG("  - species index {}", speciesIndex);
    SPDLOG_DEBUG("  - species Id {}", speciesID.toStdString());
    SPDLOG_DEBUG("  - compartment index {}", compartmentIndex);
    SPDLOG_DEBUG("  - compartment Id {}", compartmentID.toStdString());
    ui->btnRemoveSpecies->setEnabled(true);
    // display species information
    auto &field = sbmlDoc.mapSpeciesIdToField.at(speciesID);
    ui->txtSpeciesName->setText(current->text(0));
    ui->cmbSpeciesCompartment->setCurrentIndex(compartmentIndex);
    // spatial
    bool isSpatial = field.isSpatial;
    ui->chkSpeciesIsSpatial->setChecked(isSpatial);
    ui->txtDiffusionConstant->setEnabled(isSpatial);
    ui->radInitialConcentrationAnalytic->setEnabled(isSpatial);
    ui->btnEditAnalyticConcentration->setEnabled(isSpatial);
    ui->radInitialConcentrationImage->setEnabled(isSpatial);
    ui->btnEditImageConcentration->setEnabled(isSpatial);
    // constant
    bool isConstant = sbmlDoc.getIsSpeciesConstant(speciesID.toStdString());
    ui->chkSpeciesIsConstant->setChecked(isConstant);
    if (isConstant) {
      ui->txtDiffusionConstant->setEnabled(false);
      ui->lblDiffusionConstantUnits->setText("");
    }
    // initial concentration
    ui->txtInitialConcentration->setText("");
    ui->lblInitialConcentrationUnits->setText("");
    ui->lblGeometryStatus->setText(
        QString("Species '%1' concentration:").arg(current->text(0)));
    ui->lblGeometry->setImage(sbmlDoc.getConcentrationImage(speciesID));
    if (field.isUniformConcentration) {
      // scalar
      ui->txtInitialConcentration->setText(
          QString::number(sbmlDoc.getInitialConcentration(speciesID)));
      ui->lblInitialConcentrationUnits->setText(
          sbmlDoc.getModelUnits().getConcentration());
      ui->radInitialConcentrationUniform->setChecked(true);
    } else if (!sbmlDoc
                    .getSpeciesSampledFieldInitialAssignment(
                        speciesID.toStdString())
                    .empty()) {
      // image
      ui->radInitialConcentrationImage->setChecked(true);
    } else {
      // analytic
      ui->radInitialConcentrationAnalytic->setChecked(true);
    }
    radInitialConcentration_toggled();
    // diffusion constant
    if (ui->txtDiffusionConstant->isEnabled()) {
      ui->txtDiffusionConstant->setText(
          QString::number(sbmlDoc.getDiffusionConstant(speciesID)));
      ui->lblDiffusionConstantUnits->setText(
          sbmlDoc.getModelUnits().getDiffusion());
    }
    // colour
    lblSpeciesColourPixmap.fill(sbmlDoc.getSpeciesColour(speciesID));
    ui->lblSpeciesColour->setPixmap(lblSpeciesColourPixmap);
    ui->lblSpeciesColour->setText("");
  } else {
    ui->btnRemoveSpecies->setEnabled(false);
  }
}

void MainWindow::btnAddSpecies_clicked() {
  // get currently selected compartment
  int compartmentIndex = 0;
  if (auto *item = ui->listSpecies->currentItem(); item != nullptr) {
    auto *parent = item->parent() != nullptr ? item->parent() : item;
    compartmentIndex = ui->listSpecies->indexOfTopLevelItem(parent);
  }
  QString compartmentID = sbmlDoc.compartments.at(compartmentIndex);
  bool ok;
  auto speciesName = QInputDialog::getText(
      this, "Add species", "New species name:", QLineEdit::Normal, {}, &ok);
  if (ok) {
    sbmlDoc.addSpecies(speciesName, compartmentID);
    tabMain_updateSpecies();
  }
}

void MainWindow::btnRemoveSpecies_clicked() {
  if (auto *item = ui->listSpecies->currentItem();
      (item != nullptr) && (item->parent() != nullptr)) {
    SPDLOG_DEBUG("item {} / {} selected", item->parent()->text(0).toStdString(),
                 item->text(0).toStdString());
    int compartmentIndex = ui->listSpecies->indexOfTopLevelItem(item->parent());
    QString compartmentID = sbmlDoc.compartments.at(compartmentIndex);
    int speciesIndex = item->parent()->indexOfChild(item);
    QString speciesID = sbmlDoc.species.at(compartmentID).at(speciesIndex);
    if (QMessageBox::question(
            this, "Remove species",
            QString("Remove species '%1' from the model?").arg(item->text(0)),
            QMessageBox::Yes | QMessageBox::No,
            QMessageBox::Yes) == QMessageBox::Yes) {
      sbmlDoc.removeSpecies(speciesID);
      tabMain_updateSpecies();
    }
  }
}

void MainWindow::txtSpeciesName_editingFinished() {
  const QString &name = ui->txtSpeciesName->text();
  sbmlDoc.setSpeciesName(sbmlDoc.currentSpecies, name);
  tabMain_updateSpecies();
}

void MainWindow::cmbSpeciesCompartment_activated(int index) {
  const auto &currentComp =
      sbmlDoc.getSpeciesCompartment(sbmlDoc.currentSpecies);
  if (sbmlDoc.compartments[index] != currentComp) {
    sbmlDoc.setSpeciesCompartment(sbmlDoc.currentSpecies,
                                  sbmlDoc.compartments[index]);
    tabMain_updateSpecies();
  }
}

void MainWindow::chkSpeciesIsSpatial_toggled(bool enabled) {
  const auto &speciesID = sbmlDoc.currentSpecies;
  // if new value differs from previous one - update model
  if (sbmlDoc.getIsSpatial(speciesID) != enabled) {
    SPDLOG_INFO("setting species {} isSpatial: {}", speciesID.toStdString(),
                enabled);
    sbmlDoc.setIsSpatial(speciesID, enabled);
    // update displayed info for this species
    txtInitialConcentration_editingFinished();
  }
}

void MainWindow::chkSpeciesIsConstant_toggled(bool enabled) {
  const auto &speciesID = sbmlDoc.currentSpecies.toStdString();
  // if new value differs from previous one - update model
  if (sbmlDoc.getIsSpeciesConstant(speciesID) != enabled) {
    SPDLOG_INFO("setting species {} isConstant: {}", speciesID, enabled);
    sbmlDoc.setIsSpeciesConstant(speciesID, enabled);
    // update displayed info for this species
    txtInitialConcentration_editingFinished();
  }
}

void MainWindow::radInitialConcentration_toggled() {
  if (ui->radInitialConcentrationUniform->isChecked()) {
    ui->txtInitialConcentration->setEnabled(true);
    ui->btnEditAnalyticConcentration->setEnabled(false);
    ui->btnEditImageConcentration->setEnabled(false);
  } else if (ui->radInitialConcentrationImage->isChecked()) {
    ui->txtInitialConcentration->setEnabled(false);
    ui->btnEditAnalyticConcentration->setEnabled(false);
    ui->btnEditImageConcentration->setEnabled(true);
  } else {
    ui->txtInitialConcentration->setEnabled(false);
    ui->btnEditAnalyticConcentration->setEnabled(true);
    ui->btnEditImageConcentration->setEnabled(false);
  }
}

void MainWindow::txtInitialConcentration_editingFinished() {
  double initConc = ui->txtInitialConcentration->text().toDouble();
  const auto &speciesID = sbmlDoc.currentSpecies;
  SPDLOG_INFO("setting initial concentration of Species {} to {}",
              speciesID.toStdString(), initConc);
  sbmlDoc.setInitialConcentration(speciesID, initConc);
  // update displayed info for this species
  listSpecies_currentItemChanged(ui->listSpecies->currentItem(), nullptr);
}

void MainWindow::btnEditAnalyticConcentration_clicked() {
  const auto &speciesID = sbmlDoc.currentSpecies;
  SPDLOG_DEBUG("editing analytic initial concentration of species {}...",
               speciesID.toStdString());
  DialogAnalytic dialog(sbmlDoc.getAnalyticConcentration(speciesID),
                        sbmlDoc.getSpeciesGeometry(speciesID));
  if (dialog.exec() == QDialog::Accepted) {
    const std::string &expr = dialog.getExpression();
    SPDLOG_DEBUG("  - set expr: {}", expr);
    sbmlDoc.setAnalyticConcentration(speciesID, expr.c_str());
    ui->lblGeometry->setImage(sbmlDoc.getConcentrationImage(speciesID));
  }
}

void MainWindow::btnEditImageConcentration_clicked() {
  const auto &speciesID = sbmlDoc.currentSpecies;
  SPDLOG_DEBUG("editing initial concentration image for species {}...",
               speciesID.toStdString());
  DialogConcentrationImage dialog(
      sbmlDoc.getSampledFieldConcentration(speciesID),
      sbmlDoc.getSpeciesGeometry(speciesID));
  if (dialog.exec() == QDialog::Accepted) {
    SPDLOG_DEBUG("  - setting new sampled field concentration array");
    sbmlDoc.setSampledFieldConcentration(speciesID,
                                         dialog.getConcentrationArray());
    ui->lblGeometry->setImage(sbmlDoc.getConcentrationImage(speciesID));
  }
}

void MainWindow::txtDiffusionConstant_editingFinished() {
  double diffConst = ui->txtDiffusionConstant->text().toDouble();
  const auto &speciesID = sbmlDoc.currentSpecies;
  SPDLOG_INFO("setting Diffusion Constant of Species {} to {}",
              speciesID.toStdString(), diffConst);
  sbmlDoc.setDiffusionConstant(speciesID, diffConst);
}

void MainWindow::btnChangeSpeciesColour_clicked() {
  const auto &speciesID = sbmlDoc.currentSpecies;
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
  ui->txtReactionName->clear();
  ui->listReactionSpecies->clear();
  ui->listReactionParams->setRowCount(0);
  ui->txtReactionRate->clear();
  ui->lblReactionRateStatus->clear();
  ui->btnSaveReactionChanges->setEnabled(false);
  if (current != nullptr && current->parent() == nullptr) {
    // user selected a compartment or membrane: update image
    int i = ui->listReactions->indexOfTopLevelItem(current);
    if (i < sbmlDoc.compartments.size()) {
      ui->lblGeometry->setImage(
          sbmlDoc.mapCompIdToGeometry.at(sbmlDoc.compartments.at(i))
              .getCompartmentImage());
    } else {
      i -= sbmlDoc.compartments.size();
      ui->lblGeometry->setImage(
          sbmlDoc.getMembraneImage(sbmlDoc.membranes.at(i)));
    }
  }
  if ((current == nullptr) || (current->parent() == nullptr)) {
    // selection if any is not a reaction (i.e. an item with a parent)
    ui->btnAddReactionParam->setEnabled(false);
    ui->btnRemoveReactionParam->setEnabled(false);
    ui->btnRemoveReaction->setEnabled(false);
    ui->cmbReactionLocation->setEnabled(false);
    return;
  }
  SPDLOG_DEBUG("item {} / {} selected",
               current->parent()->text(0).toStdString(),
               current->text(0).toStdString());
  ui->btnAddReactionParam->setEnabled(true);
  ui->btnRemoveReaction->setEnabled(true);
  bool isMembrane = false;
  int locationIndex = ui->listReactions->indexOfTopLevelItem(current->parent());
  int compartmentIndex = locationIndex;
  if (compartmentIndex >= sbmlDoc.compartments.size()) {
    isMembrane = true;
    compartmentIndex -= sbmlDoc.compartments.size();
  }
  QString compartmentID;
  if (isMembrane) {
    compartmentID = sbmlDoc.membranes.at(compartmentIndex);
    ui->lblGeometry->setImage(sbmlDoc.getMembraneImage(compartmentID));
  } else {
    compartmentID = sbmlDoc.compartments.at(compartmentIndex);
    ui->lblGeometry->setImage(
        sbmlDoc.mapCompIdToGeometry.at(compartmentID).getCompartmentImage());
  }
  int reactionIndex = current->parent()->indexOfChild(current);
  QString reactionID = sbmlDoc.reactions.at(compartmentID).at(reactionIndex);
  SPDLOG_DEBUG("  - reaction index {}", reactionIndex);
  SPDLOG_DEBUG("  - reaction Id {}", reactionID.toStdString());
  SPDLOG_DEBUG("  - compartment index {}", compartmentIndex);
  SPDLOG_DEBUG("  - compartment Id {}", compartmentID.toStdString());

  // display reaction information
  currentReac = sbmlDoc.getReaction(reactionID);
  currentReacLocIndex = locationIndex;
  ui->txtReactionName->setText(currentReac.name.c_str());
  ui->cmbReactionLocation->setEnabled(true);
  ui->cmbReactionLocation->setCurrentIndex(locationIndex);
  ui->txtReactionRate->clearVariables();
  // species stoich
  for (const auto &compID : currentReac.compartments) {
    auto *comp = new QTreeWidgetItem;
    comp->setText(0, sbmlDoc.getCompartmentName(compID.c_str()));
    ui->listReactionSpecies->addTopLevelItem(comp);
    for (const auto &s : sbmlDoc.species.at(compID.c_str())) {
      ui->txtReactionRate->addVariable(s.toStdString(),
                                       sbmlDoc.getSpeciesName(s).toStdString());
      auto *item = new QTreeWidgetItem;
      item->setText(0, sbmlDoc.getSpeciesName(s));
      auto *spinBox = new QSpinBox(ui->listReactionSpecies);
      spinBox->setRange(-99, 99);
      comp->addChild(item);
      ui->listReactionSpecies->setItemWidget(item, 1, spinBox);
      connect(spinBox, qOverload<int>(&QSpinBox::valueChanged),
              [lst = ui->listReactionSpecies, item](int i) {
                if (i == 0) {
                  item->setData(0, Qt::BackgroundRole, {});
                  item->setData(1, Qt::BackgroundRole, {});
                } else {
                  QColor col = i > 0 ? Qt::green : Qt::red;
                  item->setData(0, Qt::BackgroundRole, QBrush(col));
                  item->setData(1, Qt::BackgroundRole, QBrush(col));
                }
              });
      for (const auto &[speciesID, name, stoich] : currentReac.species) {
        if (speciesID == s.toStdString()) {
          spinBox->setValue(static_cast<int>(stoich));
        }
      }
    }
  }
  ui->listReactionSpecies->expandAll();
  for (const auto &[id, name, value] : currentReac.constants) {
    auto *itemName = new QTableWidgetItem(name.c_str());
    itemName->setFlags(Qt::ItemIsSelectable | Qt::ItemIsEnabled);
    auto *itemValue =
        new QTableWidgetItem(QString("%1").arg(value, 14, 'g', 14));
    itemValue->setFlags(Qt::ItemIsSelectable | Qt::ItemIsEnabled |
                        Qt::ItemIsEditable);
    int n = ui->listReactionParams->rowCount();
    ui->listReactionParams->setRowCount(n + 1);
    ui->listReactionParams->setItem(n, 0, itemName);
    ui->listReactionParams->setItem(n, 1, itemValue);
    ui->txtReactionRate->addVariable(id, name);
  }
  ui->txtReactionRate->importVariableMath(currentReac.expression.c_str());
}

void MainWindow::btnAddReaction_clicked() {
  // get currently selected compartment
  int index = 0;
  if (auto *item = ui->listSpecies->currentItem(); item != nullptr) {
    auto *parent = item->parent() != nullptr ? item->parent() : item;
    index = ui->listSpecies->indexOfTopLevelItem(parent);
  }
  QString locationId;
  int nComps = sbmlDoc.compartments.size();
  if (index < nComps) {
    locationId = sbmlDoc.compartments.at(index);
  } else {
    // if a membrane, then just use the first compartment
    locationId = sbmlDoc.compartments[0];
  }
  bool ok;
  auto reactionName = QInputDialog::getText(
      this, "Add reaction", "New reaction name:", QLineEdit::Normal, {}, &ok);
  if (ok) {
    sbmlDoc.addReaction(reactionName, locationId);
    tabMain_updateReactions();  // todo: select added reaction
  }
}

void MainWindow::btnRemoveReaction_clicked() {
  if (QMessageBox::question(this, "Remove reaction",
                            QString("Remove reaction '%1'?")
                                .arg(ui->listReactions->currentItem()->text(0)),
                            QMessageBox::Yes | QMessageBox::No,
                            QMessageBox::Yes) == QMessageBox::Yes) {
    SPDLOG_INFO("Removing reaction {}", currentReac.id);
    sbmlDoc.removeReaction(currentReac.id.c_str());
    tabMain_updateReactions();
  }
}

void MainWindow::cmbReactionLocation_activated(int index) {
  QString locationId;
  int nComps = sbmlDoc.compartments.size();
  if (index < nComps) {
    locationId = sbmlDoc.compartments.at(index);
  } else {
    locationId = sbmlDoc.membranes.at(index - nComps);
  }
  if (locationId.toStdString() == currentReac.locationId) {
    return;
  }
  if (QMessageBox::question(
          this, "Change reaction location",
          QString("Change reaction location? (Species stoichiometry and rate "
                  "equation will be removed)?")
              .arg(ui->listReactions->currentItem()->text(0)),
          QMessageBox::Yes | QMessageBox::No,
          QMessageBox::Yes) == QMessageBox::Yes) {
    sbmlDoc.setReactionLocation(currentReac.id.c_str(), locationId);
    currentReacLocIndex = index;
    currentReac.locationId = locationId.toStdString();
    tabMain_updateReactions();  // todo: select this reaction
  } else {
    // reset location
    ui->cmbReactionLocation->setCurrentIndex(currentReacLocIndex);
  }
}

void MainWindow::listReactionParams_currentCellChanged(int currentRow,
                                                       int currentColumn,
                                                       int previousRow,
                                                       int previousColumn) {
  Q_UNUSED(currentColumn);
  Q_UNUSED(previousRow);
  Q_UNUSED(previousColumn);
  bool valid =
      (currentRow >= 0) && (currentRow < ui->listReactionParams->rowCount());
  ui->btnRemoveReactionParam->setEnabled(valid);
}

void MainWindow::btnAddReactionParam_clicked() {
  bool ok;
  auto name =
      QInputDialog::getText(this, "Add reaction parameter",
                            "New parameter name:", QLineEdit::Normal, {}, &ok);
  if (ok && !name.isEmpty()) {
    SPDLOG_INFO("Adding reaction parameter {}", name.toStdString());
    auto id = sbmlDoc.nameToSId(name);
    auto *itemName = new QTableWidgetItem(name);
    itemName->setFlags(Qt::ItemIsSelectable | Qt::ItemIsEnabled);
    auto *itemValue = new QTableWidgetItem("0.0");
    itemValue->setFlags(Qt::ItemIsSelectable | Qt::ItemIsEnabled |
                        Qt::ItemIsEditable);
    int n = ui->listReactionParams->rowCount();
    ui->listReactionParams->setRowCount(n + 1);
    ui->listReactionParams->setItem(n, 0, itemName);
    ui->listReactionParams->setItem(n, 1, itemValue);
    ui->txtReactionRate->addVariable(id.toStdString(), name.toStdString());
    currentReac.constants.push_back(
        {id.toStdString(), name.toStdString(), 0.0});
    ui->listReactionParams->setCurrentItem(itemValue);
  }
}

void MainWindow::btnRemoveReactionParam_clicked() {
  int row = ui->listReactionParams->currentRow();
  if ((row < 0) || (row > ui->listReactionParams->rowCount() - 1)) {
    return;
  }
  const auto &param = ui->listReactionParams->item(row, 0)->text();
  if (QMessageBox::question(
          this, "Remove reaction parameter",
          QString("Remove reaction parameter '%1'?").arg(param),
          QMessageBox::Yes | QMessageBox::No,
          QMessageBox::Yes) == QMessageBox::Yes) {
    SPDLOG_INFO("Removing parameter {}", param.toStdString());
    auto id = currentReac.constants.at(static_cast<std::size_t>(row)).id;
    currentReac.constants.erase(currentReac.constants.begin() + row);
    ui->listReactionParams->removeRow(row);
    ui->txtReactionRate->removeVariable(id);
  }
}

void MainWindow::txtReactionRate_mathChanged(const QString &math, bool valid,
                                             const QString &errorMessage) {
  ui->btnSaveReactionChanges->setEnabled(valid);
  if (valid) {
    SPDLOG_INFO("new math: {}", math.toStdString());
    ui->lblReactionRateStatus->setText("");
  } else {
    SPDLOG_INFO("math err: {}", errorMessage.toStdString());
    ui->lblReactionRateStatus->setText(errorMessage);
  }
}

void MainWindow::btnSaveReactionChanges_clicked() {
  if (!ui->txtReactionRate->mathIsValid()) {
    return;
  }
  currentReac.name = ui->txtReactionName->text().toStdString();
  int index = ui->cmbReactionLocation->currentIndex();
  QString locationId;
  int nComps = sbmlDoc.compartments.size();
  if (index < nComps) {
    locationId = sbmlDoc.compartments.at(index);
  } else {
    locationId = sbmlDoc.membranes.at(index - nComps);
  }
  currentReac.locationId = locationId.toStdString();
  // get reactants/products
  currentReac.species.clear();
  const auto *lst = ui->listReactionSpecies;
  for (int iLoc = 0; iLoc < lst->topLevelItemCount(); ++iLoc) {
    auto *comp = lst->topLevelItem(iLoc);
    std::string compartmentId =
        currentReac.compartments[static_cast<std::size_t>(iLoc)];
    SPDLOG_INFO("compartmentId: {}", compartmentId);
    const auto &species = sbmlDoc.species.at(compartmentId.c_str());
    for (int iSpec = 0; iSpec < comp->childCount(); ++iSpec) {
      auto *item = comp->child(iSpec);
      std::string speciesId = species.at(iSpec).toStdString();
      auto speciesName =
          sbmlDoc.getSpeciesName(speciesId.c_str()).toStdString();
      int stoich = 0;
      if (auto *spin = qobject_cast<QSpinBox *>(lst->itemWidget(item, 1));
          spin != nullptr) {
        stoich = spin->value();
      }
      currentReac.species.push_back(
          {speciesId, speciesName, static_cast<double>(stoich)});
      SPDLOG_INFO("- speciesId {}: stoich {}", speciesId, stoich);
    }
  }
  // add parameter values
  for (int i = 0; i < ui->listReactionParams->rowCount(); ++i) {
    auto &param = currentReac.constants.at(static_cast<std::size_t>(i));
    param.value = ui->listReactionParams->item(i, 1)->text().toDouble();
    SPDLOG_INFO("param {} = {}", param.id, param.value);
    // todo: add proper checking of numerical value
  }
  // set expression
  currentReac.expression = ui->txtReactionRate->getMath().toStdString();
  sbmlDoc.setReaction(currentReac);
  //  tabMain_updateReactions();
}

void MainWindow::listFunctions_currentRowChanged(int row) {
  ui->txtFunctionName->clear();
  ui->listFunctionParams->clear();
  ui->txtFunctionDef->clear();
  ui->lblFunctionDefStatus->clear();
  ui->btnSaveFunctionChanges->setEnabled(false);
  if ((row < 0) || (row > sbmlDoc.functions.size() - 1)) {
    ui->btnAddFunctionParam->setEnabled(false);
    ui->btnRemoveFunctionParam->setEnabled(false);
    ui->btnRemoveFunction->setEnabled(false);
    return;
  }
  auto funcId = sbmlDoc.functions.at(row);
  SPDLOG_DEBUG("Function {} selected", funcId.toStdString());
  auto func = sbmlDoc.getFunctionDefinition(funcId);
  ui->txtFunctionName->setText(func.name.c_str());
  ui->txtFunctionDef->setVariables(func.arguments);
  for (const auto &argument : func.arguments) {
    ui->listFunctionParams->addItem(argument.c_str());
  }
  if (!func.arguments.empty()) {
    ui->listFunctionParams->setCurrentRow(0);
  } else {
    ui->btnRemoveFunctionParam->setEnabled(false);
  }
  ui->txtFunctionDef->setPlainText(func.expression.c_str());
  ui->btnAddFunctionParam->setEnabled(true);
  ui->btnRemoveFunctionParam->setEnabled(!func.arguments.empty());
  ui->btnRemoveFunction->setEnabled(true);
}

void MainWindow::btnAddFunction_clicked() {
  bool ok;
  auto functionName = QInputDialog::getText(
      this, "Add function", "New function name:", QLineEdit::Normal, {}, &ok);
  if (ok) {
    sbmlDoc.addFunction(functionName);
    tabMain_updateFunctions(functionName);
  }
}

void MainWindow::btnRemoveFunction_clicked() {
  int row = ui->listFunctions->currentRow();
  if ((row < 0) || (row > sbmlDoc.functions.size() - 1)) {
    return;
  }
  if (QMessageBox::question(this, "Remove function",
                            QString("Remove function '%1' from the model?")
                                .arg(ui->listFunctions->currentItem()->text()),
                            QMessageBox::Yes | QMessageBox::No,
                            QMessageBox::Yes) == QMessageBox::Yes) {
    sbmlDoc.removeFunction(sbmlDoc.functions.at(row));
    tabMain_updateFunctions();
  }
}

void MainWindow::listFunctionParams_currentRowChanged(int row) {
  bool valid = (row >= 0) && (row < ui->listFunctionParams->count());
  ui->btnRemoveFunctionParam->setEnabled(valid);
}

void MainWindow::btnAddFunctionParam_clicked() {
  bool ok;
  auto param =
      QInputDialog::getText(this, "Add function parameter",
                            "New parameter name:", QLineEdit::Normal, {}, &ok);
  if (ok && !param.isEmpty()) {
    SPDLOG_INFO("Adding parameter {}", param.toStdString());
    SPDLOG_INFO("- todo: check valid alphanumeric variable name");
    ui->listFunctionParams->addItem(param);
    ui->txtFunctionDef->addVariable(param.toStdString());
    ui->listFunctionParams->setCurrentRow(ui->listFunctionParams->count() - 1);
  }
}

void MainWindow::btnRemoveFunctionParam_clicked() {
  int row = ui->listFunctionParams->currentRow();
  if ((row < 0) || (row > ui->listFunctionParams->count() - 1)) {
    return;
  }
  auto *item = ui->listFunctionParams->currentItem();
  const auto param = item->text();
  if (QMessageBox::question(
          this, "Remove function parameter",
          QString("Remove function parameter '%1'?").arg(param),
          QMessageBox::Yes | QMessageBox::No,
          QMessageBox::Yes) == QMessageBox::Yes) {
    SPDLOG_INFO("Removing parameter {}", param.toStdString());
    delete item;
    ui->txtFunctionDef->removeVariable(param.toStdString());
  }
}

void MainWindow::txtFunctionDef_mathChanged(const QString &math, bool valid,
                                            const QString &errorMessage) {
  ui->btnSaveFunctionChanges->setEnabled(valid);
  if (valid) {
    SPDLOG_INFO("new math: {}", math.toStdString());
    ui->lblFunctionDefStatus->setText("");
  } else {
    SPDLOG_INFO("math err: {}", errorMessage.toStdString());
    ui->lblFunctionDefStatus->setText(errorMessage);
  }
}

void MainWindow::btnSaveFunctionChanges_clicked() {
  if (!ui->txtFunctionDef->mathIsValid()) {
    return;
  }
  int row = ui->listFunctions->currentRow();
  auto func = sbmlDoc.getFunctionDefinition(sbmlDoc.functions.at(row));
  SPDLOG_INFO("Updating function {}", func.id);
  func.name = ui->txtFunctionName->text().toStdString();
  SPDLOG_INFO("  - name: {}", func.name);
  int nParams = ui->listFunctionParams->count();
  func.arguments.clear();
  func.arguments.reserve(static_cast<std::size_t>(nParams));
  for (int i = 0; i < nParams; ++i) {
    func.arguments.push_back(
        ui->listFunctionParams->item(i)->text().toStdString());
    SPDLOG_INFO("  - arg: {}", func.arguments.back());
  }
  func.expression = ui->txtFunctionDef->getMath().toStdString();
  SPDLOG_INFO("  - expression: {}", func.expression);
  sbmlDoc.setFunctionDefinition(func);
  tabMain_updateFunctions(func.name.c_str());
}

void MainWindow::btnSimulate_clicked() {
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
                               ui->lblGeometry->size());

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
  ui->lblGeometry->setImage(images.back());
  ui->statusBar->showMessage("Simulating...     (press ctrl+c to cancel)");

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
    graph->setName(sbmlDoc.getSpeciesName(sim.speciesID[i].c_str()));
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
