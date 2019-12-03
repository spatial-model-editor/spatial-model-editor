// a class that provides a pointer to various Widgets in MainWindow::ui

#pragma once

#include <qcustomplot.h>

#include <QCheckBox>
#include <QComboBox>
#include <QDebug>
#include <QDoubleSpinBox>
#include <QListWidget>
#include <QPushButton>
#include <QRadioButton>
#include <QSpinBox>
#include <QTreeWidget>

#include "mainwindow.hpp"
#include "qlabelmousetracker.hpp"
#include "qplaintextmathedit.hpp"

// macro to print variable name
#define UIPOINTER_STRINGIFY_(x) #x
// macro to call init with pointer and pointer variable name
#define UIPOINTER_INIT_(x) (init(x, UIPOINTER_STRINGIFY_(x)))
class UIPointers {
 private:
  MainWindow *w;
  template <typename T>
  void init(T &ptr, const QString &name) {
    ptr = w->topLevelWidget()->findChild<T>(name);
    if (ptr == nullptr) {
      qWarning() << "UIPointers::" << name << "not found";
    };
  }

 public:
  explicit UIPointers(MainWindow *mainWindow);
  // menus
  QMenu *menuFile;
  QMenu *menuOpen_example_SBML_file;
  QMenu *menuImport;
  QMenu *menuExample_geometry_image;
  QMenu *menu_Tools;
  QMenu *menuSimulation_type;
  QMenu *menu_Help;
  // main tab
  QTabWidget *tabMain;
  // geometry
  QWidget *tabGeometry;
  QLabelMouseTracker *lblGeometry;
  QListWidget *listCompartments;
  QLineEdit *txtCompartmentSize;
  QLabel *lblCompartmentSizeUnits;
  QPushButton *btnSetCompartmentSizeFromImage;
  QLabel *lblCompartmentColour;
  QTabWidget *tabCompartmentGeometry;
  QWidget *tabCompartmentImage;
  QLabelMouseTracker *lblCompShape;
  QPushButton *btnChangeCompartment;
  QWidget *tabCompartmentBoundary;
  QLabelMouseTracker *lblCompBoundary;
  QSpinBox *spinBoundaryIndex;
  QSpinBox *spinMaxBoundaryPoints;
  QDoubleSpinBox *spinBoundaryWidth;
  QWidget *tabCompartmentMesh;
  QLabelMouseTracker *lblCompMesh;
  QSpinBox *spinMaxTriangleArea;
  // membranes
  QWidget *tabMembranes;
  QListWidget *listMembranes;
  // species
  QWidget *tabSpecies;
  QTreeWidget *listSpecies;
  QPushButton *btnAddSpecies;
  QPushButton *btnRemoveSpecies;
  QLineEdit *txtSpeciesName;
  QComboBox *cmbSpeciesCompartment;
  QCheckBox *chkSpeciesIsSpatial;
  QCheckBox *chkSpeciesIsConstant;
  QRadioButton *radInitialConcentrationUniform;
  QLineEdit *txtInitialConcentration;
  QRadioButton *radInitialConcentrationAnalytic;
  QPushButton *btnEditAnalyticConcentration;
  QRadioButton *radInitialConcentrationImage;
  QPushButton *btnEditImageConcentration;
  QLineEdit *txtDiffusionConstant;
  QLabel *lblSpeciesColour;
  QPushButton *btnChangeSpeciesColour;
  // reactions
  QWidget *tabReactions;
  QTreeWidget *listReactions;
  QPushButton *btnAddReaction;
  QPushButton *btnRemoveReaction;
  QLineEdit *txtReactionName;
  QComboBox *cmbReactionLocation;
  QTreeWidget *listReactionSpecies;
  QTableWidget *listReactionParams;
  QPushButton *btnAddReactionParam;
  QPushButton *btnRemoveReactionParam;
  QPlainTextMathEdit *txtReactionRate;
  QPushButton *btnSaveReactionChanges;
  // functions
  QWidget *tabFunctions;
  QListWidget *listFunctions;
  QPushButton *btnAddFunction;
  QPushButton *btnRemoveFunction;
  QLineEdit *txtFunctionName;
  QListWidget *listFunctionParams;
  QPushButton *btnAddFunctionParam;
  QPushButton *btnRemoveFunctionParam;
  QPlainTextMathEdit *txtFunctionDef;
  QPushButton *btnSaveFunctionChanges;
  // simulate
  QWidget *tabSimulate;
  QLineEdit *txtSimLength;
  QLineEdit *txtSimInterval;
  QLineEdit *txtSimDt;
  QPushButton *btnSimulate;
  QPushButton *btnResetSimulation;
  QCustomPlot *pltPlot;
  // other tabs
  QWidget *tabSBML;
  QWidget *tabDUNE;
  QWidget *tabGMSH;
};

UIPointers::UIPointers(MainWindow *mainWindow) : w(mainWindow) {
  // NB this boilerplate generated from above using
  // grep -v "//" t | tr -d "*;" | awk '{print "UIPOINTER_INIT_(",$2,");"}'
  UIPOINTER_INIT_(menuFile);
  UIPOINTER_INIT_(menuOpen_example_SBML_file);
  UIPOINTER_INIT_(menuImport);
  UIPOINTER_INIT_(menuExample_geometry_image);
  UIPOINTER_INIT_(menu_Tools);
  UIPOINTER_INIT_(menuSimulation_type);
  UIPOINTER_INIT_(menu_Help);
  UIPOINTER_INIT_(tabMain);
  UIPOINTER_INIT_(tabGeometry);
  UIPOINTER_INIT_(lblGeometry);
  UIPOINTER_INIT_(listCompartments);
  UIPOINTER_INIT_(txtCompartmentSize);
  UIPOINTER_INIT_(lblCompartmentSizeUnits);
  UIPOINTER_INIT_(btnSetCompartmentSizeFromImage);
  UIPOINTER_INIT_(lblCompartmentColour);
  UIPOINTER_INIT_(tabCompartmentGeometry);
  UIPOINTER_INIT_(tabCompartmentImage);
  UIPOINTER_INIT_(lblCompShape);
  UIPOINTER_INIT_(btnChangeCompartment);
  UIPOINTER_INIT_(tabCompartmentBoundary);
  UIPOINTER_INIT_(lblCompBoundary);
  UIPOINTER_INIT_(spinBoundaryIndex);
  UIPOINTER_INIT_(spinMaxBoundaryPoints);
  UIPOINTER_INIT_(spinBoundaryWidth);
  UIPOINTER_INIT_(tabCompartmentMesh);
  UIPOINTER_INIT_(lblCompMesh);
  UIPOINTER_INIT_(spinMaxTriangleArea);
  UIPOINTER_INIT_(tabMembranes);
  UIPOINTER_INIT_(listMembranes);
  UIPOINTER_INIT_(tabSpecies);
  UIPOINTER_INIT_(listSpecies);
  UIPOINTER_INIT_(btnAddSpecies);
  UIPOINTER_INIT_(btnRemoveSpecies);
  UIPOINTER_INIT_(txtSpeciesName);
  UIPOINTER_INIT_(cmbSpeciesCompartment);
  UIPOINTER_INIT_(chkSpeciesIsSpatial);
  UIPOINTER_INIT_(chkSpeciesIsConstant);
  UIPOINTER_INIT_(radInitialConcentrationUniform);
  UIPOINTER_INIT_(txtInitialConcentration);
  UIPOINTER_INIT_(radInitialConcentrationAnalytic);
  UIPOINTER_INIT_(btnEditAnalyticConcentration);
  UIPOINTER_INIT_(radInitialConcentrationImage);
  UIPOINTER_INIT_(btnEditImageConcentration);
  UIPOINTER_INIT_(txtDiffusionConstant);
  UIPOINTER_INIT_(lblSpeciesColour);
  UIPOINTER_INIT_(btnChangeSpeciesColour);
  UIPOINTER_INIT_(tabReactions);
  UIPOINTER_INIT_(listReactions);
  UIPOINTER_INIT_(btnAddReaction);
  UIPOINTER_INIT_(btnRemoveReaction);
  UIPOINTER_INIT_(txtReactionName);
  UIPOINTER_INIT_(cmbReactionLocation);
  UIPOINTER_INIT_(txtReactionRate);
  UIPOINTER_INIT_(btnSaveReactionChanges);
  UIPOINTER_INIT_(listReactionSpecies);
  UIPOINTER_INIT_(listReactionParams);
  UIPOINTER_INIT_(btnAddReactionParam);
  UIPOINTER_INIT_(btnRemoveReactionParam);
  UIPOINTER_INIT_(tabFunctions);
  UIPOINTER_INIT_(listFunctions);
  UIPOINTER_INIT_(btnAddFunction);
  UIPOINTER_INIT_(btnRemoveFunction);
  UIPOINTER_INIT_(txtFunctionName);
  UIPOINTER_INIT_(listFunctionParams);
  UIPOINTER_INIT_(btnAddFunctionParam);
  UIPOINTER_INIT_(btnRemoveFunctionParam);
  UIPOINTER_INIT_(txtFunctionDef);
  UIPOINTER_INIT_(btnSaveFunctionChanges);
  UIPOINTER_INIT_(tabSimulate);
  UIPOINTER_INIT_(txtSimLength);
  UIPOINTER_INIT_(txtSimInterval);
  UIPOINTER_INIT_(txtSimDt);
  UIPOINTER_INIT_(btnSimulate);
  UIPOINTER_INIT_(btnResetSimulation);
  UIPOINTER_INIT_(pltPlot);
  UIPOINTER_INIT_(tabSBML);
  UIPOINTER_INIT_(tabDUNE);
  UIPOINTER_INIT_(tabGMSH);
}
