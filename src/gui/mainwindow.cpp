#include "mainwindow.hpp"

#include <QDesktopServices>
#include <QErrorMessage>
#include <QFileDialog>
#include <QInputDialog>
#include <QMessageBox>
#include <QShortcut>
#include <QWhatsThis>

#include "dialogabout.hpp"
#include "dialogimagesize.hpp"
#include "dialogintegratoroptions.hpp"
#include "dialogunits.hpp"
#include "duneini.hpp"
#include "guiutils.hpp"
#include "logger.hpp"
#include "mesh.hpp"
#include "sbml.hpp"
#include "tabfunctions.hpp"
#include "tabgeometry.hpp"
#include "tabmembranes.hpp"
#include "tabreactions.hpp"
#include "tabsimulate.hpp"
#include "tabspecies.hpp"
#include "ui_mainwindow.h"
#include "utils.hpp"
#include "version.hpp"

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent), ui{std::make_unique<Ui::MainWindow>()} {
  ui->setupUi(this);
  Q_INIT_RESOURCE(resources);

  statusBarPermanentMessage = new QLabel(QString(), this);
  ui->statusBar->addWidget(statusBarPermanentMessage);

  tabGeometry = new TabGeometry(sbmlDoc, ui->lblGeometry,
                                statusBarPermanentMessage, ui->tabReactions);
  ui->tabGeometry->layout()->addWidget(tabGeometry);

  tabMembranes = new TabMembranes(sbmlDoc, ui->tabMembranes);
  ui->tabMembranes->layout()->addWidget(tabMembranes);

  tabSpecies = new TabSpecies(sbmlDoc, ui->lblGeometry, ui->tabSpecies);
  ui->tabSpecies->layout()->addWidget(tabSpecies);

  tabReactions = new TabReactions(sbmlDoc, ui->lblGeometry, ui->tabReactions);
  ui->tabReactions->layout()->addWidget(tabReactions);

  tabFunctions = new TabFunctions(sbmlDoc, ui->tabFunctions);
  ui->tabFunctions->layout()->addWidget(tabFunctions);

  tabSimulate = new TabSimulate(sbmlDoc, ui->lblGeometry, ui->tabSimulate);
  ui->tabSimulate->layout()->addWidget(tabSimulate);

  setupConnections();

  // set initial splitter position: 1/4 for image, 3/4 for tabs
  ui->splitter->setSizes({1000, 3000});

  // load empty model by default
  sbmlDoc.createSBMLFile("untitled-model");
  this->setWindowTitle(
      QString("Spatial Model Editor [%1]").arg(sbmlDoc.currentFilename));

  enableTabs();
  ui->tabMain->setCurrentIndex(0);
  tabMain_currentChanged(0);
}

MainWindow::~MainWindow() = default;

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

  connect(ui->actionGeometry_from_image, &QAction::triggered, this,
          &MainWindow::actionGeometry_from_image_triggered);

  connect(ui->menuExample_geometry_image, &QMenu::triggered, this,
          &MainWindow::menuExample_geometry_image_triggered);

  connect(ui->actionSet_model_units, &QAction::triggered, this,
          &MainWindow::actionSet_model_units_triggered);

  connect(ui->actionSet_image_size, &QAction::triggered, this,
          &MainWindow::actionSet_image_size_triggered);

  connect(ui->actionIntegrator_options, &QAction::triggered, this,
          &MainWindow::actionIntegrator_options_triggered);

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
  switch (index) {
    case TabIndex::GEOMETRY:
      tabGeometry->loadModelData();
      break;
    case TabIndex::MEMBRANES:
      tabMembranes->loadModelData();
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
    case TabIndex::SIMULATE:
      tabSimulate->loadModelData();
      break;
    case TabIndex::SBML:
      ui->txtSBML->setText(sbmlDoc.getXml());
      break;
    case TabIndex::DUNE:
      ui->txtDUNE->setText(dune::DuneConverter(sbmlDoc).getIniFile());
      break;
    case TabIndex::GMSH:
      ui->txtGMSH->setText(sbmlDoc.mesh == nullptr ? ""
                                                   : sbmlDoc.mesh->getGMSH());
      break;
    default:
      SPDLOG_ERROR("Tab index {} not valid", index);
  }
}

void MainWindow::enableTabs() {
  bool enable = sbmlDoc.isValid && sbmlDoc.hasValidGeometry;
  for (int i = 1; i < ui->tabMain->count(); ++i) {
    ui->tabMain->setTabEnabled(i, enable);
  }
  tabGeometry->enableTabs(enable);
}

void MainWindow::action_New_triggered() {
  bool ok;
  auto modelName = QInputDialog::getText(
      this, "New model", "New model name:", QLineEdit::Normal, {}, &ok);
  if (ok && !modelName.isEmpty()) {
    sbmlDoc = sbml::SbmlDocWrapper();
    sbmlDoc.createSBMLFile(modelName.toStdString());
    tabSimulate->reset();
    ui->tabMain->setCurrentIndex(0);
    tabMain_currentChanged(0);
    enableTabs();
    this->setWindowTitle(
        QString("Spatial Model Editor [%1]").arg(sbmlDoc.currentFilename));
  }
}

void MainWindow::action_Open_SBML_file_triggered() {
  QString filename = QFileDialog::getOpenFileName(
      this, "Open SBML file", "", "SBML file (*.xml);; All files (*.*)",
      nullptr, QFileDialog::Option::DontUseNativeDialog);
  if (!filename.isEmpty()) {
    sbmlDoc = sbml::SbmlDocWrapper();
    sbmlDoc.importSBMLFile(filename.toStdString());
    if (sbmlDoc.isValid) {
      tabSimulate->reset();
      ui->tabMain->setCurrentIndex(0);
      tabMain_currentChanged(0);
      enableTabs();
      this->setWindowTitle(
          QString("Spatial Model Editor [%1]").arg(sbmlDoc.currentFilename));
    }
  }
}

void MainWindow::menuOpen_example_SBML_file_triggered(const QAction *action) {
  QString filename =
      QString(":/models/%1.xml").arg(action->text().remove(0, 1));
  QFile f(filename);
  if (!f.open(QIODevice::ReadOnly)) {
    SPDLOG_WARN("failed to open built-in file: {}", filename.toStdString());
    return;
  }
  sbmlDoc = sbml::SbmlDocWrapper();
  sbmlDoc.importSBMLString(f.readAll().toStdString());
  tabSimulate->reset();
  ui->tabMain->setCurrentIndex(0);
  tabMain_currentChanged(0);
  enableTabs();
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
  sbmlDoc.importGeometryFromImage(image);
  ui->tabMain->setCurrentIndex(0);
  tabMain_currentChanged(0);
  enableTabs();
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

void MainWindow::actionIntegrator_options_triggered() {
  DialogIntegratorOptions dialog(tabSimulate->getIntegratorOptions());
  if (dialog.exec() == QDialog::Accepted) {
    tabSimulate->setIntegratorOptions(dialog.getIntegratorOptions());
  }
}

bool MainWindow::isValidModel() {
  if (sbmlDoc.isValid) {
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
  if (sbmlDoc.hasGeometryImage) {
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
