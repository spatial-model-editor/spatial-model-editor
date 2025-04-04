#pragma once

#include "sme/image_stack.hpp"
#include "sme/model.hpp"
#include <QLabel>
#include <QMainWindow>
#include <memory>

class TabFunctions;
class TabGeometry;
class TabEvents;
class TabParameters;
class TabReactions;
class TabSimulate;
class TabSpecies;

namespace Ui {
class MainWindow;
}

class MainWindow : public QMainWindow {
  Q_OBJECT

public:
  explicit MainWindow(const QString &filename = {}, QWidget *parent = nullptr);
  ~MainWindow() override;

private:
  std::unique_ptr<Ui::MainWindow> ui;
  QLabel *statusBarPermanentMessage;
  sme::model::Model model;
  bool haveCopasiSE{false};

  void setupTabs();
  void setupConnections();

  // check if SBML model and geometry image are both valid
  // offer user to load a valid one if not
  bool isValidModel();
  bool isValidModelAndGeometryImage();
  void importGeometryImage(const sme::common::ImageStack &image,
                           bool is_model_image = false);
  void tabMain_currentChanged(int index);
  TabGeometry *tabGeometry;
  TabSpecies *tabSpecies;
  TabReactions *tabReactions;
  TabFunctions *tabFunctions;
  TabParameters *tabParameters;
  TabEvents *tabEvents;
  TabSimulate *tabSimulate;

  QString getConvertedFilename(const QString &cpsFilename);
  void validateSBMLDoc(const QString &filename = {});
  // if SBML model and geometry are both valid, enable all tabs
  void enableTabs();

  // File menu actions
  void action_New_triggered();
  void action_Open_SBML_file_triggered();
  void menuOpen_example_SBML_file_triggered(const QAction *action);
  void action_Save_triggered();
  void action_Save_SBML_file_triggered();
  void actionExport_Dune_ini_file_triggered();

  // Import menu actions
  void actionGeometry_from_model_triggered();
  void actionGeometry_from_image_triggered();
  void menuExample_geometry_image_triggered(const QAction *action);

  // Tools menu actions
  void actionSet_model_units_triggered();
  void actionEdit_geometry_image_triggered();
  void actionSet_spatial_coordinates_triggered();
  void actionFinalize_non_spatial_import_triggered();
  void action_Optimization_triggered();
  void action_steadystate_analysis_triggered();

  // View menu actions
  void actionGeometry_grid_triggered(bool checked);
  void actionGeometry_scale_triggered(bool checked);
  void actionInvert_y_axis_triggered(bool checked);
  void action3d_render_triggered(bool checked);

  // Advanced menu actions
  void actionSimulation_options_triggered();
  void action_Meshing_options_triggered();

  void lblGeometry_mouseOver(const sme::common::Voxel &voxel);
  void spinGeometryZoom_valueChanged(int value);

  void dragEnterEvent(QDragEnterEvent *event) override;
  void dropEvent(QDropEvent *event) override;
  void closeEvent(QCloseEvent *event) override;
};
