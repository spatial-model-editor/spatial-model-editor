#include "mainwindow.hpp"

#include <QDesktopServices>
#include <QErrorMessage>
#include <QFileDialog>
#include <QInputDialog>
#include <QMessageBox>
#include <QMimeData>
#include <QShortcut>
#include <QWhatsThis>

#include "dialogabout.hpp"
#include "dialogcoordinates.hpp"
#include "dialogimagesize.hpp"
#include "dialogintegratoroptions.hpp"
#include "dialogunits.hpp"
#include "duneconverter.hpp"
#include "guiutils.hpp"
#include "logger.hpp"
#include "mesh.hpp"
#include "model.hpp"
#include "tabfunctions.hpp"
#include "tabgeometry.hpp"
#include "tabparameters.hpp"
#include "tabreactions.hpp"
#include "tabsimulate.hpp"
#include "tabspecies.hpp"
#include "ui_mainwindow.h"
#include "utils.hpp"
#include "version.hpp"

MainWindow::MainWindow(const QString &filename, QWidget *parent)
    : QMainWindow(parent), ui{std::make_unique<Ui::MainWindow>()} {
  ui->setupUi(this);
  Q_INIT_RESOURCE(resources);

  statusBarPermanentMessage = new QLabel(QString(), this);
  ui->statusBar->addWidget(statusBarPermanentMessage);

  setupTabs();
  setupConnections();

  // set initial splitter position: 1/4 for image, 3/4 for tabs
  ui->splitter->setSizes({1000, 3000});

  if (!filename.isEmpty()) {
    openSBMLFile(filename);
  } else {
    validateSBMLDoc();
  }
}

MainWindow::~MainWindow() = default;

void MainWindow::setupTabs() {
  tabGeometry = new TabGeometry(sbmlDoc, ui->lblGeometry,
                                statusBarPermanentMessage, ui->tabReactions);
  ui->tabGeometry->layout()->addWidget(tabGeometry);

  tabSpecies = new TabSpecies(sbmlDoc, ui->lblGeometry, ui->tabSpecies);
  ui->tabSpecies->layout()->addWidget(tabSpecies);

  tabReactions = new TabReactions(sbmlDoc, ui->lblGeometry, ui->tabReactions);
  ui->tabReactions->layout()->addWidget(tabReactions);

  tabFunctions = new TabFunctions(sbmlDoc, ui->tabFunctions);
  ui->tabFunctions->layout()->addWidget(tabFunctions);

  tabParameters = new TabParameters(sbmlDoc, ui->tabParameters);
  ui->tabParameters->layout()->addWidget(tabParameters);

  tabSimulate = new TabSimulate(sbmlDoc, ui->lblGeometry, ui->tabSimulate);
  ui->tabSimulate->layout()->addWidget(tabSimulate);
}

void MainWindow::setupConnections() {
  // menu actions
  connect(ui->action_New, &QAction::triggered, this,
          &MainWindow::action_New_triggered);

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

  connect(ui->actionGeometry_from_model, &QAction::triggered, this,
          &MainWindow::actionGeometry_from_model_triggered);

  connect(ui->actionGeometry_from_image, &QAction::triggered, this,
          &MainWindow::actionGeometry_from_image_triggered);

  connect(ui->menuExample_geometry_image, &QMenu::triggered, this,
          &MainWindow::menuExample_geometry_image_triggered);

  connect(ui->actionSet_model_units, &QAction::triggered, this,
          &MainWindow::actionSet_model_units_triggered);

  connect(ui->actionSet_image_size, &QAction::triggered, this,
          &MainWindow::actionSet_image_size_triggered);

  connect(ui->actionSet_spatial_coordinates, &QAction::triggered, this,
          &MainWindow::actionSet_spatial_coordinates_triggered);

  connect(ui->actionIntegrator_options, &QAction::triggered, this,
          &MainWindow::actionIntegrator_options_triggered);

  connect(ui->actionMax_cpu_threads, &QAction::triggered, this,
          &MainWindow::actionMax_cpu_threads_triggered);

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

  // tabs
  connect(ui->tabMain, &QTabWidget::currentChanged, this,
          &MainWindow::tabMain_currentChanged);

  connect(tabGeometry, &TabGeometry::invalidModelOrNoGeometryImage, this,
          &MainWindow::isValidModelAndGeometryImage);

  connect(tabGeometry, &TabGeometry::modelGeometryChanged, this,
          &MainWindow::enableTabs);

  connect(ui->actionGroupSimType, &QActionGroup::triggered, this,
          [s = tabSimulate, ui = ui.get()](QAction *action) {
            Q_UNUSED(action);
            s->useDune(ui->actionSimTypeDUNE->isChecked());
          });
}

void MainWindow::tabMain_currentChanged(int index) {
  enum TabIndex {
    GEOMETRY = 0,
    SPECIES = 1,
    REACTIONS = 2,
    FUNCTIONS = 3,
    PARAMETERS = 4,
    SIMULATE = 5,
    SBML = 6,
    DUNE = 7,
    GMSH = 8
  };
  ui->tabMain->setWhatsThis(ui->tabMain->tabWhatsThis(index));
  SPDLOG_DEBUG("Tab changed to {} [{}]", index,
               ui->tabMain->tabText(index).toStdString());
  switch (index) {
  case TabIndex::GEOMETRY:
    tabGeometry->loadModelData();
    break;
  case TabIndex::SPECIES:
    tabSpecies->loadModelData();
    break;
  case TabIndex::REACTIONS:
    tabReactions->loadModelData();
    break;
  case TabIndex::FUNCTIONS:
    tabFunctions->loadModelData();
    break;
  case TabIndex::PARAMETERS:
    tabParameters->loadModelData();
    break;
  case TabIndex::SIMULATE:
    tabSimulate->loadModelData();
    break;
  case TabIndex::SBML:
    ui->txtSBML->setText(sbmlDoc.getXml());
    break;
  case TabIndex::DUNE:
    ui->txtDUNE->setText(simulate::DuneConverter(sbmlDoc).getIniFile());
    break;
  case TabIndex::GMSH:
    ui->txtGMSH->setText(sbmlDoc.getGeometry().getMesh() == nullptr
                             ? ""
                             : sbmlDoc.getGeometry().getMesh()->getGMSH());
    break;
  default:
    SPDLOG_ERROR("Tab index {} not valid", index);
  }
}

void MainWindow::validateSBMLDoc(const QString &filename) {
  if (!sbmlDoc.getIsValid()) {
    sbmlDoc.createSBMLFile("untitled-model");
    if (!filename.isEmpty()) {
      QMessageBox::warning(this, "Failed to load file",
                           "Failed to load file " + filename);
    }
  }
  tabSimulate->reset();
  ui->tabMain->setCurrentIndex(0);
  tabMain_currentChanged(0);
  enableTabs();
  this->setWindowTitle(
      QString("Spatial Model Editor [%1]").arg(sbmlDoc.getCurrentFilename()));
}

void MainWindow::enableTabs() {
  bool enable = sbmlDoc.getIsValid() && sbmlDoc.getGeometry().getIsValid();
  for (int i = 1; i < ui->tabMain->count(); ++i) {
    ui->tabMain->setTabEnabled(i, enable);
  }
  tabGeometry->enableTabs();
}

void MainWindow::action_New_triggered() {
  bool ok;
  auto modelName = QInputDialog::getText(
      this, "Create new model", "New model name:", QLineEdit::Normal, {}, &ok);
  if (ok && !modelName.isEmpty()) {
    sbmlDoc.createSBMLFile(modelName.toStdString());
    validateSBMLDoc();
  }
}

void MainWindow::action_Open_SBML_file_triggered() {
  QString filename = QFileDialog::getOpenFileName(
      this, "Open SBML file", "", "SBML file (*.xml);; All files (*.*)");
  if (filename.isEmpty()) {
    return;
  }
  openSBMLFile(filename);
}

void MainWindow::menuOpen_example_SBML_file_triggered(const QAction *action) {
  QString filename =
      QString(":/models/%1.xml").arg(action->text().remove(0, 1));
  QFile f(filename);
  if (!f.open(QIODevice::ReadOnly)) {
    SPDLOG_WARN("failed to open built-in file: {}", filename.toStdString());
    return;
  }
  sbmlDoc.importSBMLString(f.readAll().toStdString());
  validateSBMLDoc(filename);
}

void MainWindow::action_Save_SBML_file_triggered() {
  if (!isValidModelAndGeometryImage()) {
    SPDLOG_DEBUG("invalid geometry and/or model: ignoring");
    return;
  }
  QString filename = QFileDialog::getSaveFileName(this, "Save SBML file",
                                                  sbmlDoc.getCurrentFilename(),
                                                  "SBML file (*.xml)");
  if (filename.isEmpty()) {
    return;
  }
  if (filename.right(4) != ".xml") {
    filename.append(".xml");
  }
  sbmlDoc.exportSBMLFile(filename.toStdString());
  this->setWindowTitle(
      QString("Spatial Model Editor [%1]").arg(sbmlDoc.getCurrentFilename()));
}

void MainWindow::actionExport_Dune_ini_file_triggered() {
  if (!isValidModelAndGeometryImage()) {
    SPDLOG_DEBUG("invalid geometry and/or model: ignoring");
    return;
  }
  QString filename = QFileDialog::getSaveFileName(this, "Export DUNE ini file",
                                                  "", "DUNE ini file (*.ini)");
  if (!filename.isEmpty()) {
    if (filename.right(4) != ".ini") {
      filename.append(".ini");
    }
    simulate::DuneConverter dc(sbmlDoc, true);
    QFile f(filename);
    if (f.open(QIODevice::ReadWrite | QIODevice::Text)) {
      f.write(dc.getIniFile().toUtf8());
    }
    // also export gmsh file `grid.msh` in the same dir
    QString dir = QFileInfo(filename).absolutePath();
    QString meshFilename = QDir(dir).filePath("grid.msh");
    QFile f2(meshFilename);
    if (f2.open(QIODevice::ReadWrite | QIODevice::Text)) {
      f2.write(sbmlDoc.getGeometry()
                   .getMesh()
                   ->getGMSH(dc.getGMSHCompIndices())
                   .toUtf8());
    }
  }
}

void MainWindow::actionGeometry_from_model_triggered() {
  if (!isValidModel()) {
    return;
  }
  QString filename =
      QFileDialog::getOpenFileName(this, "Import geometry from SBML file", "",
                                   "SBML file (*.xml);; All files (*.*)");
  if (filename.isEmpty()) {
    return;
  }
  tabSimulate->reset();
  for (const auto &id : sbmlDoc.getCompartments().getIds()) {
    sbmlDoc.getCompartments().setColour(id, 0);
  }
  sbmlDoc.getGeometry().importSampledFieldGeometry(filename);
  ui->tabMain->setCurrentIndex(0);
  tabMain_currentChanged(0);
  enableTabs();
}

void MainWindow::actionGeometry_from_image_triggered() {
  if (!isValidModel()) {
    return;
  }
  auto img = getImageFromUser(this, "Import geometry from image");
  if (!img.isNull()) {
    importGeometryImage(img);
  }
}

void MainWindow::menuExample_geometry_image_triggered(const QAction *action) {
  if (!isValidModel()) {
    return;
  }
  QString filename =
      QString(":/geometry/%1.png").arg(action->text().remove(0, 1));
  QImage img(filename);
  importGeometryImage(img);
}

void MainWindow::importGeometryImage(const QImage &image) {
  tabSimulate->reset();
  sbmlDoc.getGeometry().importGeometryFromImage(image);
  ui->tabMain->setCurrentIndex(0);
  tabMain_currentChanged(0);
  enableTabs();
  // set default pixelwidth in case user doesn't set image physical size
  sbmlDoc.getGeometry().setPixelWidth(1.0);
  actionSet_image_size_triggered();
}

void MainWindow::openSBMLFile(const QString &filename) {
  sbmlDoc.importSBMLFile(filename.toStdString());
  validateSBMLDoc(filename);
}

void MainWindow::actionSet_model_units_triggered() {
  if (!isValidModel()) {
    return;
  }
  model::Unit oldLengthUnit = sbmlDoc.getUnits().getLength();
  double oldPixelWidth = sbmlDoc.getGeometry().getPixelWidth();
  DialogUnits dialog(sbmlDoc.getUnits());
  if (dialog.exec() == QDialog::Accepted) {
    sbmlDoc.getUnits().setTimeIndex(dialog.getTimeUnitIndex());
    sbmlDoc.getUnits().setLengthIndex(dialog.getLengthUnitIndex());
    sbmlDoc.getUnits().setVolumeIndex(dialog.getVolumeUnitIndex());
    sbmlDoc.getUnits().setAmountIndex(dialog.getAmountUnitIndex());
    // rescale pixelsize to match new units
    sbmlDoc.getGeometry().setPixelWidth(model::rescale(
        oldPixelWidth, oldLengthUnit, sbmlDoc.getUnits().getLength()));
  }
}

void MainWindow::actionSet_image_size_triggered() {
  if (!isValidModelAndGeometryImage()) {
    return;
  }
  DialogImageSize dialog(sbmlDoc.getGeometry().getImage(),
                         sbmlDoc.getGeometry().getPixelWidth(),
                         sbmlDoc.getUnits());
  if (dialog.exec() == QDialog::Accepted) {
    double pixelWidth = dialog.getPixelWidth();
    SPDLOG_INFO("Set new pixel width = {}", pixelWidth);
    sbmlDoc.getGeometry().setPixelWidth(pixelWidth);
  }
}

void MainWindow::actionSet_spatial_coordinates_triggered() {
  if (!isValidModel()) {
    return;
  }
  auto &params = sbmlDoc.getParameters();
  auto coords = params.getSpatialCoordinates();
  DialogCoordinates dialog(coords.x.name.c_str(), coords.y.name.c_str());
  if (dialog.exec() == QDialog::Accepted) {
    coords.x.name = dialog.getXName().toStdString();
    coords.y.name = dialog.getYName().toStdString();
    params.setSpatialCoordinates(std::move(coords));
  }
}

void MainWindow::actionIntegrator_options_triggered() {
  DialogIntegratorOptions dialog(tabSimulate->getIntegratorOptions());
  if (dialog.exec() == QDialog::Accepted) {
    tabSimulate->setIntegratorOptions(dialog.getIntegratorOptions());
    tabMain_currentChanged(ui->tabMain->currentIndex());
  }
}

void MainWindow::actionMax_cpu_threads_triggered() {
  bool ok;
  auto numThreads = static_cast<std::size_t>(QInputDialog::getInt(
      this, "Set max cpu threads",
      "Max cpu threads (0 is the default and means use "
      "all available threads):",
      static_cast<int>(tabSimulate->getMaxThreads()), 0, 64, 1, &ok));
  if (ok) {
    SPDLOG_DEBUG("setting max threads to {}", numThreads);
    tabSimulate->setMaxThreads(numThreads);
  }
}

void MainWindow::dragEnterEvent(QDragEnterEvent *event) {
  event->acceptProposedAction();
}

void MainWindow::dropEvent(QDropEvent *event) {
  const QMimeData *mimeData = event->mimeData();
  if (!mimeData->hasUrls() || mimeData->urls().isEmpty()) {
    return;
  }
  openSBMLFile(mimeData->urls().front().toLocalFile());
}

bool MainWindow::isValidModel() {
  if (sbmlDoc.getIsValid()) {
    return true;
  }
  SPDLOG_DEBUG("  - no SBML model");
  auto msgbox = newYesNoMessageBox(
      "No SBML model", "No valid SBML model loaded - import one now?", this);
  connect(msgbox, &QMessageBox::finished, this, [this](int result) {
    if (result == QMessageBox::Yes) {
      action_Open_SBML_file_triggered();
    }
  });
  msgbox->open();
  return false;
}

bool MainWindow::isValidModelAndGeometryImage() {
  if (!isValidModel()) {
    return false;
  }
  if (sbmlDoc.getGeometry().getHasImage()) {
    return true;
  }
  SPDLOG_DEBUG("  - no geometry image");
  auto msgbox = newYesNoMessageBox(
      "No compartment geometry image",
      "No compartment geometry image loaded - import one now?", this);
  connect(msgbox, &QMessageBox::finished, this, [this](int result) {
    if (result == QMessageBox::Yes) {
      actionGeometry_from_image_triggered();
    }
  });
  msgbox->open();
  return false;
}
