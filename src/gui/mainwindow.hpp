#pragma once

#include <QMainWindow>
#include <memory>

#include "sbml.hpp"
class QLabel;
class QTabFunctions;
class QTabGeometry;
class QTabReactions;
class QTabSimulate;
class QTabSpecies;

namespace Ui {
class MainWindow;
}

class MainWindow : public QMainWindow {
  Q_OBJECT

 public:
  explicit MainWindow(QWidget *parent = nullptr);
  ~MainWindow();

 private:
  std::unique_ptr<Ui::MainWindow> ui;
  QLabel *statusBarPermanentMessage;
  sbml::SbmlDocWrapper sbmlDoc;

  void setupConnections();

  // check if SBML model and geometry image are both valid
  // offer user to load a valid one if not
  bool isValidModel();
  bool isValidModelAndGeometryImage();

  void importGeometryImage(const QImage &image);

  void tabMain_currentChanged(int index);
  void tabMain_updateMembranes();
  QTabGeometry *tabGeometry;
  QTabSpecies *tabSpecies;
  QTabReactions *tabReactions;
  QTabFunctions *tabFunctions;
  QTabSimulate *tabSimulate;
  // if SBML model and geometry are both valid, enable all tabs
  void enableTabs();

  // File menu actions
  void action_New_triggered();
  void action_Open_SBML_file_triggered();
  void menuOpen_example_SBML_file_triggered(const QAction *action);
  void action_Save_SBML_file_triggered();
  void actionExport_Dune_ini_file_triggered();

  // Import menu actions
  void actionGeometry_from_image_triggered();
  void menuExample_geometry_image_triggered(const QAction *action);

  // Tools menu actions
  void actionSet_model_units_triggered();
  void actionSet_image_size_triggered();

  // membranes
  void listMembranes_currentRowChanged(int currentRow);
};
