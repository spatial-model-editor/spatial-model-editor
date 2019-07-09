#pragma once

#include <QMainWindow>

#include "qcustomplot/qcustomplot.h"

#include "sbml.hpp"

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

  QPixmap lblSpeciesColourPixmap;
  QPixmap lblCompartmentColourPixmap;

  // temp: vector of simulation images to display
  QVector<QImage> images;

  void setupConnections();

  // update list of species to display in simulation result image
  void updateSpeciesDisplaySelect();

  // check if SBML model and geometry image are both valid
  // offer user to load a valid one if not
  bool isValidModelAndGeometry();

  // <UI>
  void tabMain_currentChanged(int index);
  void tabMain_updateGeometry();
  void tabMain_updateMembranes();
  void tabMain_updateSpecies();
  void tabMain_updateReactions();
  void tabMain_updateFunctions();
  void tabMain_updateSimulate();
  // if SBML model and geometry image are both valid, enable all tabs
  void enableTabs();

  // File menu actions
  void action_Open_SBML_file_triggered();

  void menuOpen_example_SBML_file_triggered(QAction *action);

  void action_Save_SBML_file_triggered();

  // Import menu actions
  void actionGeometry_from_image_triggered();

  void menuExample_geometry_image_triggered(QAction *action);

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
  void listSpecies_currentItemChanged(QTreeWidgetItem *current,
                                      QTreeWidgetItem *previous);

  void chkSpeciesIsSpatial_toggled(bool enabled);

  void chkSpeciesIsConstant_toggled(bool enabled);

  void radInitialConcentration_toggled();

  void txtInitialConcentration_editingFinished();

  void btnImportConcentration_clicked();

  void txtDiffusionConstant_editingFinished();

  void btnChangeSpeciesColour_clicked();

  // reactions
  void listReactions_currentItemChanged(QTreeWidgetItem *current,
                                        QTreeWidgetItem *previous);

  // functions
  void listFunctions_currentTextChanged(const QString &currentText);

  // simulate
  void btnSimulate_clicked();

  void graphClicked(QCPAbstractPlottable *plottable, int dataIndex);

  void hslideTime_valueChanged(int value);
  // </UI>
};
