#pragma once

#include <QMainWindow>
#include <QTreeWidgetItem>
#include "ext/qcustomplot/qcustomplot.h"

#include "sbml.h"

namespace Ui {
class MainWindow;
}

class MainWindow : public QMainWindow {
  Q_OBJECT

 public:
  explicit MainWindow(QWidget *parent = nullptr);

 private:
  std::shared_ptr<Ui::MainWindow> ui;
  SbmlDocWrapper sbmlDoc;
  bool waitingForCompartmentChoice = false;

  // temp: vector of simulation images to display
  QVector<QImage> images;

  void setupConnections();

  // update list of species to display in simulation result image
  void updateSpeciesDisplaySelect();

  // <UI>
  void tabMain_currentChanged(int index);

  void tabMain_updateGeometry();
  void tabMain_updateMembranes();
  void tabMain_updateSpecies();
  void tabMain_updateReactions();
  void tabMain_updateFunctions();
  void tabMain_updateSimulate();

  // File menu actions
  void action_Open_SBML_file_triggered();

  void action_Save_SBML_file_triggered();

  // Import menu actions
  void actionGeometry_from_image_triggered();

  // About menu actions
  void action_About_triggered();

  // geometry
  void lblGeometry_mouseClicked(QRgb col);

  void btnChangeCompartment_clicked();

  void listCompartments_currentTextChanged(const QString &currentText);

  void listCompartments_itemDoubleClicked(QListWidgetItem *item);

  // membranes
  void listMembranes_currentTextChanged(const QString &currentText);

  // species
  void listSpecies_itemActivated(QTreeWidgetItem *item, int column);

  void listSpecies_itemClicked(QTreeWidgetItem *item, int column);

  void chkSpeciesIsSpatial_stateChanged(int arg1);

  void chkShowSpatialAdvanced_stateChanged(int arg1);

  void btnImportConcentration_clicked();

  // reactions
  void listReactions_itemActivated(QTreeWidgetItem *item, int column);

  void listReactions_itemClicked(QTreeWidgetItem *item, int column);

  // functions
  void listFunctions_currentTextChanged(const QString &currentText);

  // simulate
  void btnSimulate_clicked();

  void graphClicked(QCPAbstractPlottable *plottable, int dataIndex);

  void hslideTime_valueChanged(int value);
  // </UI>
};
