#include "mainwindow.hpp"
#include "dialogabout.hpp"
#include "dialogcoordinates.hpp"
#include "dialoggeometryimage.hpp"
#include "dialogmeshingoptions.hpp"
#include "dialogoptimize.hpp"
#include "dialogoptsetup.hpp"
#include "dialogsimulationoptions.hpp"
#include "dialogsteadystate.hpp"
#include "dialogunits.hpp"
#include "guiutils.hpp"
#include "ode_import_wizard.hpp"
#include "sme/duneconverter.hpp"
#include "sme/logger.hpp"
#include "sme/mesh2d.hpp"
#include "sme/model.hpp"
#include "sme/serialization.hpp"
#include "tabevents.hpp"
#include "tabfunctions.hpp"
#include "tabgeometry.hpp"
#include "tabparameters.hpp"
#include "tabreactions.hpp"
#include "tabsimulate.hpp"
#include "tabspecies.hpp"
#include "ui_mainwindow.h"
#include <QDesktopServices>
#include <QErrorMessage>
#include <QFileDialog>
#include <QFileInfo>
#include <QInputDialog>
#include <QMessageBox>
#include <QMimeData>
#include <QProcess>
#include <QWhatsThis>
#include <spdlog/spdlog.h>

static QString getStatusBarMessage(int step) {
  if (step == 1) {
    return R"(<h2><font color = "Green">Importing non-spatial model. Step 1/3: <a href="#geometry-image">import geometry image</a></font></h2>)";
  }
  if (step == 2) {
    return R"(<h2><font color = "Green">Importing non-spatial model. Step 2/3: assign compartment geometry</font></h2>)";
  }
  if (step == 3) {
    return R"(<h2><font color = "Green">Importing non-spatial model. Step 3/3: <a href="#rescale-reactions">rescale reactions</a></font></h2>)";
  }
  return {};
};

static bool checkForCopasiSE() {
  constexpr int msTimeout{1000};
  QProcess copasi;
  copasi.setProgram("CopasiSE");
  copasi.setArguments({"--help"});
  copasi.start();
  return copasi.waitForStarted(msTimeout) && copasi.waitForFinished(msTimeout);
}

static QString getOutputXmlFilename(const QString &cpsFilename) {
  QString filename{QFileInfo(cpsFilename).baseName()};
  while (QFileInfo::exists(filename + ".xml")) {
    // ensure we don't overwrite an existing xml file
    filename.append("_");
  }
  return filename + ".xml";
}

QString MainWindow::getConvertedFilename(const QString &filename) {
  if (!haveCopasiSE || QFileInfo(filename).suffix() != "cps") {
    return filename;
  }
  constexpr int msTimeout{10000};
  auto xmlFilename{getOutputXmlFilename(filename)};
  SPDLOG_INFO("CopasiSE: {} -> {}", filename.toStdString(),
              xmlFilename.toStdString());
  QProcess copasi;
  copasi.setProgram("CopasiSE");
  copasi.setArguments({"-e", xmlFilename, "--SBMLSchema", "L3V1", filename});
  copasi.start();
  if (copasi.waitForStarted(msTimeout) && copasi.waitForFinished(msTimeout) &&
      copasi.exitCode() == 0) {
    // if successful, return sbml filename
    return xmlFilename;
  }
  // otherwise return original filename
  return filename;
}

static QLabel *makeStatusBarPermanentMessage(QWidget *parent) {
  QLabel *lbl{new QLabel(QString(), parent)};
  lbl->setTextFormat(Qt::TextFormat::RichText);
  lbl->setOpenExternalLinks(false);
  lbl->setTextInteractionFlags(
      Qt::TextInteractionFlag::LinksAccessibleByKeyboard |
      Qt::TextInteractionFlag::LinksAccessibleByMouse);
  return lbl;
}

MainWindow::MainWindow(const QString &filename, QWidget *parent)
    : QMainWindow(parent), ui{std::make_unique<Ui::MainWindow>()} {
  ui->setupUi(this);
  ui->lblGeometry->setZSlider(ui->slideGeometryZIndex);
  ui->voxGeometry->setClippingPlaneNormalCombobox(ui->cmbClippingPlaneNormal);
  ui->voxGeometry->setClippingPlaneOriginSlider(ui->slideClippingPlaneOrigin);
  Q_INIT_RESOURCE(resources);

  statusBarPermanentMessage = makeStatusBarPermanentMessage(this);
  ui->statusBar->addPermanentWidget(statusBarPermanentMessage);
  statusBarPermanentMessage->hide();

  setupTabs();
  setupConnections();

  haveCopasiSE = checkForCopasiSE();
  if (haveCopasiSE) {
    SPDLOG_INFO("Found CopasiSE: enabling .cps file import");
  }

  // set initial splitter position: 1/4 for image, 3/4 for tabs
  ui->splitter->setSizes({1000, 3000});

  if (!filename.isEmpty()) {
    auto f{getConvertedFilename(filename)};
    model.importFile(f.toStdString());
    validateSBMLDoc(f);
  } else {
    validateSBMLDoc();
  }
}

MainWindow::~MainWindow() = default;

void MainWindow::setupTabs() {
  tabGeometry = new TabGeometry(model, ui->lblGeometry, ui->voxGeometry,
                                statusBar(), ui->tabGeometry);
  ui->tabGeometry->layout()->addWidget(tabGeometry);

  tabSpecies =
      new TabSpecies(model, ui->lblGeometry, ui->voxGeometry, ui->tabSpecies);
  ui->tabSpecies->layout()->addWidget(tabSpecies);

  tabReactions = new TabReactions(model, ui->lblGeometry, ui->voxGeometry,
                                  ui->tabReactions);
  ui->tabReactions->layout()->addWidget(tabReactions);

  tabFunctions = new TabFunctions(model, ui->tabFunctions);
  ui->tabFunctions->layout()->addWidget(tabFunctions);

  tabParameters = new TabParameters(model, ui->tabParameters);
  ui->tabParameters->layout()->addWidget(tabParameters);

  tabEvents = new TabEvents(model, ui->tabEvents);
  ui->tabEvents->layout()->addWidget(tabEvents);

  tabSimulate =
      new TabSimulate(model, ui->lblGeometry, ui->voxGeometry, ui->tabSimulate);
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

  connect(ui->action_Save, &QAction::triggered, this,
          &MainWindow::action_Save_triggered);

  connect(ui->action_Save_SBML_file, &QAction::triggered, this,
          &MainWindow::action_Save_SBML_file_triggered);

  connect(ui->actionExport_Dune_ini_file, &QAction::triggered, this,
          &MainWindow::actionExport_Dune_ini_file_triggered);

  connect(ui->actionE_xit, &QAction::triggered, this,
          &QApplication::closeAllWindows);

  connect(ui->actionGeometry_from_model, &QAction::triggered, this,
          &MainWindow::actionGeometry_from_model_triggered);

  connect(ui->actionGeometry_from_image, &QAction::triggered, this,
          &MainWindow::actionGeometry_from_image_triggered);

  connect(ui->menuExample_geometry_image, &QMenu::triggered, this,
          &MainWindow::menuExample_geometry_image_triggered);

  connect(ui->actionSet_model_units, &QAction::triggered, this,
          &MainWindow::actionSet_model_units_triggered);

  connect(ui->actionEdit_geometry_image, &QAction::triggered, this,
          &MainWindow::actionEdit_geometry_image_triggered);

  connect(ui->actionSet_spatial_coordinates, &QAction::triggered, this,
          &MainWindow::actionSet_spatial_coordinates_triggered);

  connect(ui->actionSimulation_options, &QAction::triggered, this,
          &MainWindow::actionSimulation_options_triggered);

  connect(ui->action_Meshing_options, &QAction::triggered, this,
          &MainWindow::action_Meshing_options_triggered);

  connect(ui->action_Optimization, &QAction::triggered, this,
          &MainWindow::action_Optimization_triggered);

  connect(ui->actionSteady_state_analysis, &QAction::triggered, this,
          &MainWindow::action_steadystate_analysis_triggered);

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
          [s = tabSimulate, ui_ptr = ui.get()]() {
            s->useDune(ui_ptr->actionSimTypeDUNE->isChecked());
          });

  connect(ui->actionGeometry_grid, &QAction::triggered, this,
          &MainWindow::actionGeometry_grid_triggered);

  connect(ui->actionGeometry_scale, &QAction::triggered, this,
          &MainWindow::actionGeometry_scale_triggered);

  connect(ui->action3d_render, &QAction::triggered, this,
          &MainWindow::action3d_render_triggered);

  connect(ui->actionInvert_y_axis, &QAction::triggered, this,
          &MainWindow::actionInvert_y_axis_triggered);

  connect(ui->lblGeometry, &QLabelMouseTracker::mouseOver, this,
          &MainWindow::lblGeometry_mouseOver);

  connect(ui->lblGeometry, &QLabelMouseTracker::mouseWheelEvent, this,
          [this](QWheelEvent *ev) {
            if (ev->modifiers() == Qt::ShiftModifier) {
              QApplication::sendEvent(ui->spinGeometryZoom, ev);
            }
          });

  connect(ui->spinGeometryZoom, qOverload<int>(&QSpinBox::valueChanged), this,
          &MainWindow::spinGeometryZoom_valueChanged);

  connect(statusBarPermanentMessage, &QLabel::linkActivated, this,
          [this](const QString &link) {
            if (link == "#rescale-reactions") {
              return actionFinalize_non_spatial_import_triggered();
            }
            if (link == "#geometry-image") {
              return actionGeometry_from_image_triggered();
            }
          });
}

void MainWindow::tabMain_currentChanged(int index) {
  enum TabIndex {
    GEOMETRY = 0,
    SPECIES = 1,
    REACTIONS = 2,
    FUNCTIONS = 3,
    PARAMETERS = 4,
    EVENTS = 5,
    SIMULATE = 6
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
  case TabIndex::EVENTS:
    tabEvents->loadModelData();
    break;
  case TabIndex::SIMULATE:
    tabSimulate->loadModelData();
    break;
  default:
    SPDLOG_ERROR("Tab index {} not valid", index);
  }
}

void MainWindow::validateSBMLDoc(const QString &filename) {
  QString titlebarFilename{filename};
  if (!model.getIsValid()) {
    if (!filename.isEmpty() && filename.left(5) != ("-psn_")) {
      // MacOS sometimes passes a command line parameter of the form
      // `-psn_0_204850` to the executable when launched as a GUI app, so in
      // this case we don't warn the user that we can't open this non-existent
      // file
      QMessageBox::warning(this, "Failed to load file",
                           "Failed to load file " + filename + "\n\n" +
                               "Error message: " + model.getErrorMessage());
    }
    model.createSBMLFile("untitled-model");
    titlebarFilename = model.getCurrentFilename();
  }
  tabSimulate->importTimesAndIntervalsOnNextLoad();
  ui->tabMain->setCurrentIndex(0);
  tabMain_currentChanged(0);
  enableTabs();
  if (model.getSimulationSettings().simulatorType ==
      sme::simulate::SimulatorType::DUNE) {
    ui->actionSimTypeDUNE->setChecked(true);
  } else {
    ui->actionSimTypePixel->setChecked(true);
  }
  ui->actionGeometry_grid->setChecked(
      model.getDisplayOptions().showGeometryGrid);
  actionGeometry_grid_triggered(model.getDisplayOptions().showGeometryGrid);
  ui->actionGeometry_scale->setChecked(
      model.getDisplayOptions().showGeometryScale);
  actionGeometry_scale_triggered(model.getDisplayOptions().showGeometryScale);
  ui->actionInvert_y_axis->setChecked(model.getDisplayOptions().invertYAxis);
  actionInvert_y_axis_triggered(model.getDisplayOptions().invertYAxis);
  this->setWindowTitle(
      QString("Spatial Model Editor [%1]").arg(titlebarFilename));
}

void MainWindow::enableTabs() {
  bool enable{model.getIsValid() && model.getGeometry().getIsValid()};
  ui->lblGeometry->setPhysicalUnits(model.getUnits().getLength().name);
  for (int i = 1; i < ui->tabMain->count(); ++i) {
    ui->tabMain->setTabEnabled(i, enable);
  }
  if (model.getReactions().getIsIncompleteODEImport()) {
    if (model.getGeometry().getIsValid()) {
      statusBarPermanentMessage->setText(getStatusBarMessage(3));
    } else if (model.getGeometry().getHasImage()) {
      statusBarPermanentMessage->setText(getStatusBarMessage(2));
    } else {
      statusBarPermanentMessage->setText(getStatusBarMessage(1));
    }
    statusBarPermanentMessage->show();
  } else {
    statusBarPermanentMessage->hide();
  }
  tabGeometry->enableTabs();
}

void MainWindow::action_New_triggered() {
  bool ok;
  auto modelName = QInputDialog::getText(
      this, "Create new model", "New model name:", QLineEdit::Normal, {}, &ok);
  if (ok && !modelName.isEmpty()) {
    tabSimulate->reset();
    model.createSBMLFile(modelName.toStdString());
    validateSBMLDoc(modelName + ".sme");
  }
}

void MainWindow::action_Open_SBML_file_triggered() {
  QString filetypes{"SME or SBML model (*.sme *.xml)"};
  if (haveCopasiSE) {
    filetypes = "SME, SBML or COPASI model (*.sme *.xml *.cps)";
  }
  QString filename = QFileDialog::getOpenFileName(
      this, "Open Model", "", filetypes + ";; All files (*.*)");
  if (filename.isEmpty()) {
    return;
  }
  filename = getConvertedFilename(filename);
  QGuiApplication::setOverrideCursor(QCursor(Qt::WaitCursor));
  model.importFile(filename.toStdString());
  QGuiApplication::restoreOverrideCursor();
  validateSBMLDoc(filename);
  if (model.getReactions().getIsIncompleteODEImport()) {
    QMessageBox::information(
        this, "Non-spatial model import",
        "Non-spatial model imported. See the status message at the "
        "bottom-right of the "
        "window for what to do next to convert this into a spatial model.");
  }
}

void MainWindow::menuOpen_example_SBML_file_triggered(const QAction *action) {
  QString filename{action->text().remove(0, 1) + ".xml"};
  QFile f(":/models/" + filename);
  if (!f.open(QIODevice::ReadOnly)) {
    SPDLOG_WARN("failed to open built-in file: {}", filename.toStdString());
    return;
  }
  QGuiApplication::setOverrideCursor(QCursor(Qt::WaitCursor));
  model.importSBMLString(f.readAll().toStdString(), filename.toStdString());
  QGuiApplication::restoreOverrideCursor();
  validateSBMLDoc(filename);
}

void MainWindow::action_Save_triggered() {
  QString filename = QFileDialog::getSaveFileName(
      this, "Save Model", model.getCurrentFilename() + ".sme",
      "SME file (*.sme)");
  if (filename.isEmpty()) {
    return;
  }
  if (filename.right(4) != ".sme") {
    filename.append(".sme");
  }
  QGuiApplication::setOverrideCursor(QCursor(Qt::WaitCursor));
  model.exportSMEFile(filename.toStdString()); // todo check for success here
  QGuiApplication::restoreOverrideCursor();
  this->setWindowTitle(QString("Spatial Model Editor [%1]").arg(filename));
}

void MainWindow::action_Save_SBML_file_triggered() {
  QString filename = QFileDialog::getSaveFileName(
      this, "Export SBML file", model.getCurrentFilename() + ".xml",
      "SBML file (*.xml)");
  if (filename.isEmpty()) {
    return;
  }
  if (filename.right(4) != ".xml") {
    filename.append(".xml");
  }
  QGuiApplication::setOverrideCursor(QCursor(Qt::WaitCursor));
  model.exportSBMLFile(filename.toStdString()); // todo check for success here
  QGuiApplication::restoreOverrideCursor();
}

void MainWindow::actionExport_Dune_ini_file_triggered() {
  if (!isValidModelAndGeometryImage()) {
    SPDLOG_DEBUG("invalid geometry and/or model: ignoring");
    return;
  }
  QString iniFilename = QFileDialog::getSaveFileName(
      this, "Export DUNE ini file", "", "DUNE ini file (*.ini)");
  if (iniFilename.isEmpty()) {
    return;
  }
  if (iniFilename.right(4) != ".ini") {
    iniFilename.append(".ini");
  }
  QGuiApplication::setOverrideCursor(QCursor(Qt::WaitCursor));
  sme::simulate::DuneConverter dc(model, {}, true, iniFilename);
  QGuiApplication::restoreOverrideCursor();
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
  for (const auto &id : model.getCompartments().getIds()) {
    model.getCompartments().setColor(id, 0);
  }
  QGuiApplication::setOverrideCursor(QCursor(Qt::WaitCursor));
  model.getGeometry().importSampledFieldGeometry(filename);
  QGuiApplication::restoreOverrideCursor();
  ui->tabMain->setCurrentIndex(0);
  tabMain_currentChanged(0);
  enableTabs();
}

void MainWindow::actionGeometry_from_image_triggered() {
  if (!isValidModel()) {
    return;
  }
  if (auto img{getImageFromUser(this, "Import geometry from image")};
      !img.empty()) {
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
  importGeometryImage(sme::common::ImageStack({img}));
}

void MainWindow::importGeometryImage(const sme::common::ImageStack &image,
                                     bool is_model_image) {
  DialogGeometryImage dialog(image, model.getGeometry().getVoxelSize(),
                             model.getUnits());
  if (dialog.exec() == QDialog::Accepted) {
    QGuiApplication::setOverrideCursor(QCursor(Qt::WaitCursor));
    tabSimulate->reset();
    if (!is_model_image || dialog.imageSizeAltered() ||
        dialog.imageColorsAltered()) {
      SPDLOG_INFO("Importing altered geometry image");
      bool keepColorAssignments{is_model_image && !dialog.imageColorsAltered()};
      model.getGeometry().importGeometryFromImages(dialog.getAlteredImage(),
                                                   keepColorAssignments);
    }
    auto voxelSize{dialog.getVoxelSize()};
    SPDLOG_INFO("Set new voxel volume {}x{}x{}", voxelSize.width(),
                voxelSize.height(), voxelSize.depth());
    model.getGeometry().setVoxelSize(voxelSize);
    ui->tabMain->setCurrentIndex(0);
    tabMain_currentChanged(0);
    enableTabs();
    QGuiApplication::restoreOverrideCursor();
  }
}

void MainWindow::actionSet_model_units_triggered() {
  if (!isValidModel()) {
    return;
  }
  sme::model::Unit oldLengthUnit{model.getUnits().getLength()};
  auto oldVoxelSize{model.getGeometry().getVoxelSize()};
  DialogUnits dialog(model.getUnits());
  if (dialog.exec() == QDialog::Accepted) {
    model.getUnits().setTimeIndex(dialog.getTimeUnitIndex());
    model.getUnits().setLengthIndex(dialog.getLengthUnitIndex());
    model.getUnits().setVolumeIndex(dialog.getVolumeUnitIndex());
    model.getUnits().setAmountIndex(dialog.getAmountUnitIndex());
    // rescale pixelsize to match new units
    model.getGeometry().setVoxelSize(sme::model::rescale(
        oldVoxelSize, oldLengthUnit, model.getUnits().getLength()));
    enableTabs();
  }
}

void MainWindow::actionEdit_geometry_image_triggered() {
  if (!isValidModelAndGeometryImage()) {
    return;
  }
  importGeometryImage(model.getGeometry().getImages(), true);
}

void MainWindow::actionSet_spatial_coordinates_triggered() {
  if (!isValidModel()) {
    return;
  }
  auto &params{model.getParameters()};
  auto coords{params.getSpatialCoordinates()};
  DialogCoordinates dialog(coords.x.name.c_str(), coords.y.name.c_str(),
                           coords.z.name.c_str());
  if (dialog.exec() == QDialog::Accepted) {
    coords.x.name = dialog.getXName().toStdString();
    coords.y.name = dialog.getYName().toStdString();
    coords.z.name = dialog.getZName().toStdString();
    params.setSpatialCoordinates(std::move(coords));
  }
}

void MainWindow::actionFinalize_non_spatial_import_triggered() {
  OdeImportWizard wiz(model);
  if (wiz.exec() == QWizard::Accepted) {
    statusBarPermanentMessage->hide();
    model.getReactions().applySpatialReactionRescalings(
        wiz.getReactionRescalings());
  } else {
    // reset depth to previous value
    // todo: revisit this: probably broken
    const auto vs{model.getGeometry().getVoxelSize()};
    model.getGeometry().setVoxelSize(
        {vs.width(), vs.height(), wiz.getInitialPixelDepth()});
  }
}

void MainWindow::action_Optimization_triggered() {
  if (!isValidModelAndGeometryImage()) {
    SPDLOG_DEBUG("invalid geometry and/or model: ignoring");
    return;
  }
  try {
    DialogOptimize dialogOptimize(model);
    if (dialogOptimize.exec() == QDialog::Accepted) {
      auto paramsString{dialogOptimize.getParametersString()};
      if (!paramsString.isEmpty()) {
        auto result{QMessageBox::question(
            this, "Apply optimized parameters to model?",
            "Apply these optimized parameters to model?\n\n" + paramsString,
            QMessageBox::Yes | QMessageBox::No)};
        if (result == QMessageBox::Yes) {
          dialogOptimize.applyToModel();
        }
      }
    }
  } catch (const std::invalid_argument &e) {
    QMessageBox::warning(this, "Optimize error", e.what());
  }
}

void MainWindow::action_steadystate_analysis_triggered() {
  if (!isValidModelAndGeometryImage()) {
    SPDLOG_DEBUG("invalid geometry and/or model: ignoring");
    return;
  }
  DialogSteadystate dialogSteadystate(model);
  dialogSteadystate.exec();
}

void MainWindow::actionGeometry_grid_triggered(bool checked) {
  ui->lblGeometry->displayGrid(checked);
  auto options{model.getDisplayOptions()};
  options.showGeometryGrid = checked;
  model.setDisplayOptions(options);
}

void MainWindow::actionGeometry_scale_triggered(bool checked) {
  ui->lblGeometry->displayScale(checked);
  auto options{model.getDisplayOptions()};
  options.showGeometryScale = checked;
  model.setDisplayOptions(options);
}

void MainWindow::action3d_render_triggered(bool checked) {
  ui->stackGeometry->setCurrentIndex(checked ? 1 : 0);
}

void MainWindow::actionInvert_y_axis_triggered(bool checked) {
  ui->lblGeometry->invertYAxis(checked);
  tabGeometry->invertYAxis(checked);
  tabSimulate->invertYAxis(checked);
  auto options{model.getDisplayOptions()};
  options.invertYAxis = checked;
  model.setDisplayOptions(options);
}

void MainWindow::actionSimulation_options_triggered() {
  DialogSimulationOptions dialog(model.getSimulationSettings().options);
  if (dialog.exec() == QDialog::Accepted) {
    tabSimulate->setOptions(dialog.getOptions());
    tabMain_currentChanged(ui->tabMain->currentIndex());
  }
}

void MainWindow::action_Meshing_options_triggered() {
  DialogMeshingOptions dialog(model.getMeshParameters().boundarySimplifierType);
  if (auto &boundarySimplifierType{
          model.getMeshParameters().boundarySimplifierType};
      dialog.exec() == QDialog::Accepted &&
      boundarySimplifierType != dialog.getBoundarySimplificationType()) {
    boundarySimplifierType = dialog.getBoundarySimplificationType();
    model.getGeometry().updateMesh();
    tabMain_currentChanged(ui->tabMain->currentIndex());
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
  auto filename = getConvertedFilename(mimeData->urls().front().toLocalFile());
  QStringList imgSuffixes{"bmp", "png", "jpg", "jpeg", "gif", "tif", "tiff"};
  if (imgSuffixes.contains(QFileInfo(filename).suffix().toLower())) {
    if (auto img{getImageStackFromFilename(this, filename)}; !img.empty()) {
      importGeometryImage(img);
    }
  } else {
    model.importFile(filename.toStdString());
    validateSBMLDoc(filename);
  }
}

void MainWindow::closeEvent(QCloseEvent *event) {
  if (!model.getHasUnsavedChanges()) {
    event->accept();
    return;
  }
  auto res = QMessageBox::question(
      this, "Unsaved changes",
      "Model has unsaved changes. Do you want to save them before exiting?",
      QMessageBox::No | QMessageBox::Yes, QMessageBox::Yes);
  if (res == QMessageBox::Yes) {
    event->ignore();
    action_Save_triggered();
  } else {
    event->accept();
  }
}

bool MainWindow::isValidModel() {
  if (model.getIsValid()) {
    return true;
  }
  SPDLOG_DEBUG("  - no SBML model");
  auto result{QMessageBox::question(
      this, "No SBML model", "No valid SBML model loaded - import one now?",
      QMessageBox::Yes | QMessageBox::No)};
  if (result == QMessageBox::Yes) {
    action_Open_SBML_file_triggered();
  }
  return false;
}

bool MainWindow::isValidModelAndGeometryImage() {
  if (!isValidModel()) {
    return false;
  }
  if (model.getGeometry().getHasImage()) {
    return true;
  }
  SPDLOG_DEBUG("  - no geometry image");
  auto result{QMessageBox::question(
      this, "No compartment geometry image",
      "No compartment geometry image loaded - import one now?",
      QMessageBox::Yes | QMessageBox::No)};
  if (result == QMessageBox::Yes) {
    actionGeometry_from_image_triggered();
  }
  return false;
}

void MainWindow::lblGeometry_mouseOver(const sme::common::Voxel &voxel) {
  if (!model.getGeometry().getHasImage()) {
    return;
  }
  statusBar()->showMessage(model.getGeometry().getPhysicalPointAsString(voxel));
}

void MainWindow::spinGeometryZoom_valueChanged(int value) {
  if (value == 0) {
    // rescale widget to fit entire scroll region
    ui->scrollGeometry->setWidgetResizable(true);
  } else {
    zoomScrollArea(ui->scrollGeometry, value,
                   ui->lblGeometry->getRelativePosition());
  }
}
