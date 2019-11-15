#pragma once

#include <QMainWindow>

#include "sbml.hpp"
#include "simulate.hpp"

class QLabel;
class QListWidgetItem;
class QTreeWidgetItem;
class QShortcut;
class QCPAbstractPlottable;

namespace Ui {
class MainWindow;
}

class MainWindow : public QMainWindow {
  Q_OBJECT

 public:
  explicit MainWindow(QWidget *parent = nullptr);

 private:
  std::shared_ptr<Ui::MainWindow> ui;
  QLabel *statusBarPermanentMessage;
  sbml::SbmlDocWrapper sbmlDoc;
  bool waitingForCompartmentChoice = false;
  QShortcut *shortcutStopSimulation;
  bool isSimulationRunning = false;
  QShortcut *shortcutSetMathBackend;
  simulate::BACKEND simMathBackend = simulate::BACKEND::SYMENGINE_LLVM;

  bool useDuneSimulator = true;

  QPixmap lblSpeciesColourPixmap;
  QPixmap lblCompartmentColourPixmap;

  // temp: vector of simulation images to display
  QVector<QImage> images;

  void setupConnections();

  void setMathBackend();

  // update list of species to display in simulation result image
  void updateSpeciesDisplaySelect();

  // check if SBML model and geometry image are both valid
  // offer user to load a valid one if not
  bool isValidModel();
  bool isValidModelAndGeometryImage();

  void importGeometryImage(const QImage &image);

  void tabMain_currentChanged(int index);
  void tabMain_updateGeometry();
  void tabMain_updateMembranes();
  void tabMain_updateSpecies();
  void tabMain_updateReactions();
  void tabMain_updateFunctions();
  void tabMain_updateSimulate();
  // if SBML model and geometry are both valid, enable all tabs
  void enableTabs();

  // File menu actions
  void action_Open_SBML_file_triggered();

  void menuOpen_example_SBML_file_triggered(QAction *action);

  void action_Save_SBML_file_triggered();

  void actionExport_Dune_ini_file_triggered();

  // Import menu actions
  void actionGeometry_from_image_triggered();

  void menuExample_geometry_image_triggered(QAction *action);

  // Tools menu actions

  void actionSet_model_units_triggered();

  void actionSet_image_size_triggered();

  // geometry
  void lblGeometry_mouseClicked(QRgb col, QPoint point);

  void btnChangeCompartment_clicked();

  void btnSetCompartmentSizeFromImage_clicked();

  void tabCompartmentGeometry_currentChanged(int index);

  void lblCompBoundary_mouseClicked(QRgb col, QPoint point);

  void spinBoundaryIndex_valueChanged(int value);

  void spinMaxBoundaryPoints_valueChanged(int value);

  void spinBoundaryWidth_valueChanged(double value);

  void lblCompMesh_mouseClicked(QRgb col, QPoint point);

  void spinMaxTriangleArea_valueChanged(int value);

  void generateMesh(int value = 0);

  void listCompartments_currentRowChanged(int currentRow);

  void listCompartments_itemDoubleClicked(QListWidgetItem *item);

  // membranes
  void listMembranes_currentRowChanged(int currentRow);

  // species
  void listSpecies_currentItemChanged(QTreeWidgetItem *current,
                                      QTreeWidgetItem *previous);

  void chkSpeciesIsSpatial_toggled(bool enabled);

  void chkSpeciesIsConstant_toggled(bool enabled);

  void radInitialConcentration_toggled();

  void txtInitialConcentration_editingFinished();

  void btnEditAnalyticConcentration_clicked();

  void btnEditImageConcentration_clicked();

  void txtDiffusionConstant_editingFinished();

  void btnChangeSpeciesColour_clicked();

  // reactions
  void listReactions_currentItemChanged(QTreeWidgetItem *current,
                                        QTreeWidgetItem *previous);

  // functions
  void listFunctions_currentTextChanged(const QString &currentText);

  // simulate
  void btnSimulate_clicked();

  void btnResetSimulation_clicked();

  void graphClicked(QCPAbstractPlottable *plottable, int dataIndex);

  void hslideTime_valueChanged(int value);
};
