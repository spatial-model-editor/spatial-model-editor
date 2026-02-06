#include "catch_wrapper.hpp"
#include "mainwindow.hpp"
#include "model_test_utils.hpp"
#include "qlabelmousetracker.hpp"
#include "qt_test_utils.hpp"
#include <QComboBox>
#include <QFile>
#include <QMenu>
#include <QSpinBox>
#include <QStatusBar>

using namespace sme::test;
using Catch::Matchers::ContainsSubstring;

static const char *tags{"[gui/mainwindow][gui][mainwindow]"};

static void openBuiltInModel(MainWindow &w, const QString &shortcutKey = "V") {
  auto *menuFile = w.findChild<QMenu *>("menuFile");
  auto *menuOpen_example_SBML_file{
      w.findChild<QMenu *>("menuOpen_example_SBML_file")};
  sendKeyEvents(&w, {"Alt+F"});
  sendKeyEvents(menuFile, {"E"});
  sendKeyEvents(menuOpen_example_SBML_file, {shortcutKey, "Enter"});
}

TEST_CASE("MainWindow: non-existent file", tags) {
  ModalWidgetTimer mwt;
  mwt.addUserAction({"Esc"});
  mwt.start();
  MainWindow w("dontexist.xml");
  w.show();
  REQUIRE_THAT(mwt.getResult().toStdString(),
               ContainsSubstring("Failed to load file dontexist.xml"));
}

TEST_CASE("MainWindow: shortcut keys", tags) {
  MainWindow w;
  w.show();
  waitFor(&w);
  ModalWidgetTimer mwt;
  SECTION("F8") {
    mwt.start();
    sendKeyEvents(&w, {"F8"});
    REQUIRE(mwt.getResult() == "About Spatial Model Editor");
  }
#ifndef __APPLE__
  // On MacOS About QT dialog is not modal, so we skip this test
  SECTION("F9") {
    mwt.start();
    sendKeyEvents(&w, {"F9"});
    CAPTURE(mwt.getResult());
    REQUIRE_THAT(mwt.getResult().toStdString(), ContainsSubstring("About Qt"));
  }
#endif
}

TEST_CASE("MainWindow: new file shortcut keys", tags) {
  MainWindow w;
  w.show();
  waitFor(&w);
  ModalWidgetTimer mwt;
  // ctrl+n to create new model, then escape to cancel
  SECTION("cancel") {
    QString oldTitle{w.windowTitle()};
    mwt.addUserAction({"Escape"}, false);
    mwt.start();
    sendKeyEvents(&w, {"Ctrl+N"});
    REQUIRE(mwt.getResult() == "Create new model");
    REQUIRE(w.windowTitle() == oldTitle);
  }
  SECTION("new") {
    // ctrl+n to create new model with name "new"
    mwt.addUserAction({"n", "e", "w"});
    mwt.start();
    sendKeyEvents(&w, {"Ctrl+N"});
    REQUIRE(mwt.getResult() == "Create new model");
    REQUIRE(w.windowTitle().right(9) == "[new.sme]");
  }
}

TEST_CASE("MainWindow: open/save shortcut keys", tags) {
  MainWindow w;
  w.show();
  waitFor(&w);
  ModalWidgetTimer mwt;
  SECTION("user presses ctrl+s (with valid SBML model)") {
    openBuiltInModel(w, "V");
    SECTION("cancel") {
      mwt.addUserAction({"Escape"}, false);
      mwt.start();
      sendKeyEvents(&w, {"Ctrl+S"});
      REQUIRE(mwt.getResult() == "QFileDialog::AcceptSave");
    }
    SECTION("save sme file") {
      QFile::remove("tmpmainw1.sme");
      mwt.addUserAction({"t", "m", "p", "m", "a", "i", "n", "w", "1"});
      mwt.start();
      sendKeyEvents(&w, {"Ctrl+S"});
      REQUIRE(mwt.getResult() == "QFileDialog::AcceptSave");
    }
  }
  SECTION("user presses ctrl+e (with valid SBML model)") {
    openBuiltInModel(w, "V");
    SECTION("cancel") {
      mwt.addUserAction({"Escape"}, false);
      mwt.start();
      sendKeyEvents(&w, {"Ctrl+E"});
      REQUIRE(mwt.getResult() == "QFileDialog::AcceptSave");
    }
    SECTION("save sbml xml file") {
      QFile::remove("tmpmainw1.xml");
      mwt.addUserAction({"t", "m", "p", "m", "a", "i", "n", "w", "1"});
      mwt.start();
      sendKeyEvents(&w, {"Ctrl+E"});
      REQUIRE(mwt.getResult() == "QFileDialog::AcceptSave");
      QFile file("tmpmainw1.xml");
      REQUIRE(file.open(QIODevice::ReadOnly | QIODevice::Text));
      auto line = file.readLine().toStdString();
      REQUIRE(line == "<?xml version=\"1.0\" encoding=\"UTF-8\"?>\n");
    }
  }
  SECTION("user presses ctrl+d (SBML model but no geometry loaded)") {
    SECTION("offer to import a geometry image") {
      mwt.addUserAction({"Escape"});
      mwt.start();
      sendKeyEvents(&w, {"Ctrl+D"});
      // press no when asked to import image
      REQUIRE(mwt.getResult() ==
              "No compartment geometry image loaded - import one now?");
    }
  }
  SECTION("user presses ctrl+d (with valid SBML model)") {
    SECTION("cancel") {
      openBuiltInModel(w, "A");
      mwt.addUserAction({"Escape"});
      mwt.start();
      sendKeyEvents(&w, {"Ctrl+D"});
      REQUIRE(mwt.getResult() == "QFileDialog::AcceptSave");
    }
    SECTION("save dune ini file") {
      openBuiltInModel(w, "A");
      QFile::remove("tmpmainw1_comp.ini");
      mwt.addUserAction({"t", "m", "p", "m", "a", "i", "n", "w", "1"});
      mwt.start();
      sendKeyEvents(&w, {"Ctrl+D"});
      REQUIRE(mwt.getResult() == "QFileDialog::AcceptSave");
      QFile file("tmpmainw1.ini");
      REQUIRE(file.open(QIODevice::ReadOnly | QIODevice::Text));
      auto line = file.readLine().toStdString();
      REQUIRE(line == "[grid]\n");
    }
  }
  SECTION("Ctrl+O") {
    SECTION("cancel") {
      // ctrl+o to open model, then escape to cancel
      mwt.addUserAction({"Esc"}, false);
      mwt.start();
      sendKeyEvents(&w, {"Ctrl+O"});
      REQUIRE(mwt.getResult() == "QFileDialog::AcceptOpen");
    }
    SECTION("open sbml xml file") {
      REQUIRE(w.windowTitle() == "Spatial Model Editor [untitled-model]");
      mwt.addUserAction(
          {"t", "m", "p", "m", "a", "i", "n", "w", "1", ".", "x", "m", "l"});
      mwt.start();
      sendKeyEvents(&w, {"Ctrl+O"});
      REQUIRE(mwt.getResult() == "QFileDialog::AcceptOpen");
      REQUIRE(w.windowTitle().right(14) == "tmpmainw1.xml]");
    }
    SECTION("open sme file") {
      REQUIRE(w.windowTitle() == "Spatial Model Editor [untitled-model]");
      mwt.addUserAction(
          {"t", "m", "p", "m", "a", "i", "n", "w", "1", ".", "s", "m", "e"});
      mwt.start();
      sendKeyEvents(&w, {"Ctrl+O"});
      REQUIRE(mwt.getResult() == "QFileDialog::AcceptOpen");
      REQUIRE(w.windowTitle().right(14) == "tmpmainw1.sme]");
    }
  }
  SECTION("user presses ctrl+I to import image (default empty SBML model)") {
    SECTION("offer to import SBML model") {
      // escape on SBML file open dialog
      mwt.addUserAction({"Esc"}, false);
      mwt.start();
      sendKeyEvents(&w, {"Ctrl+I"});
      REQUIRE(mwt.getResult() == "QFileDialog::AcceptOpen");
    }
  }
}

TEST_CASE("MainWindow: tabs", tags) {
  MainWindow w;
  auto *tabMain{w.findChild<QTabWidget *>("tabMain")};
  REQUIRE(tabMain != nullptr);
  w.show();
  waitFor(&w);
  SECTION("ctrl+tab (default SBML model loaded)") {
    // remain on Geometry tab: all others disabled
    REQUIRE(tabMain->currentIndex() == 0);
    sendKeyEvents(tabMain, {"Ctrl+Tab"});
    REQUIRE(tabMain->currentIndex() == 0);
  }
  SECTION("ctrl+shift+tab (default SBML model loaded)") {
    // remain on Geometry tab: all others disabled"
    REQUIRE(tabMain->currentIndex() == 0);
    sendKeyEvents(tabMain, {"Ctrl+Shift+Tab"});
    REQUIRE(tabMain->currentIndex() == 0);
  }
  SECTION("ctrl+tab with valid model loaded") {
    openBuiltInModel(w, "V");
    REQUIRE(tabMain->currentIndex() == 0);
    sendKeyEvents(tabMain, {"Ctrl+Tab"});
    REQUIRE(tabMain->currentIndex() == 1);
    sendKeyEvents(tabMain, {"Ctrl+Tab"});
    REQUIRE(tabMain->currentIndex() == 2);
    sendKeyEvents(tabMain, {"Ctrl+Tab"});
    REQUIRE(tabMain->currentIndex() == 3);
    sendKeyEvents(tabMain, {"Ctrl+Tab"});
    REQUIRE(tabMain->currentIndex() == 4);
    sendKeyEvents(tabMain, {"Ctrl+Tab"});
    REQUIRE(tabMain->currentIndex() == 5);
    sendKeyEvents(tabMain, {"Ctrl+Tab"});
    REQUIRE(tabMain->currentIndex() == 6);
    sendKeyEvents(tabMain, {"Ctrl+Tab"});
    REQUIRE(tabMain->currentIndex() == 0);
    sendKeyEvents(tabMain, {"Ctrl+Tab"});
    REQUIRE(tabMain->currentIndex() == 1);
    sendKeyEvents(tabMain, {"Ctrl+Tab"});
    REQUIRE(tabMain->currentIndex() == 2);
    sendKeyEvents(tabMain, {"Ctrl+Shift+Tab"});
    REQUIRE(tabMain->currentIndex() == 1);
    sendKeyEvents(tabMain, {"Ctrl+Shift+Tab"});
    REQUIRE(tabMain->currentIndex() == 0);
    sendKeyEvents(tabMain, {"Ctrl+Shift+Tab"});
    REQUIRE(tabMain->currentIndex() == 6);
  }
}

TEST_CASE("MainWindow: tools menu", tags) {
  MainWindow w;
  auto *menu_Tools{w.findChild<QMenu *>("menu_Tools")};
  REQUIRE(menu_Tools != nullptr);
  auto *menuSimulation_type{w.findChild<QMenu *>("menuSimulation_type")};
  REQUIRE(menuSimulation_type != nullptr);
  auto *actionSimTypeDUNE{w.findChild<QAction *>("actionSimTypeDUNE")};
  REQUIRE(actionSimTypeDUNE != nullptr);
  auto *actionSimTypePixel{w.findChild<QAction *>("actionSimTypePixel")};
  REQUIRE(actionSimTypePixel != nullptr);
  auto *actionSteady_state_analysis{
      w.findChild<QAction *>("actionSteady_state_analysis")};
  REQUIRE(actionSteady_state_analysis != nullptr);
  w.show();
  waitFor(&w);
  ModalWidgetTimer mwt;
  SECTION("menu: Tools->Set model units (default SBML model)") {
    SECTION("offer to import SBML model") {
      // press esc to close dialog
      mwt.addUserAction({"Esc"}, false);
      mwt.start();
      sendKeyEvents(&w, {"Alt+T"});
      sendKeyEvents(menu_Tools, {"U"});
      REQUIRE(mwt.getResult() == "Set Model Units");
    }
  }
  SECTION("menu: Tools->Edit geometry image (default SBML model, no image)") {
    SECTION("offer to import geometry image") {
      mwt.addUserAction({"Esc"});
      mwt.start();
      sendKeyEvents(&w, {"Alt+T"});
      sendKeyEvents(menu_Tools, {"E"});
      // press no when asked to import image
      REQUIRE(mwt.getResult() ==
              "No compartment geometry image loaded - import one now?");
    }
  }
  SECTION("menu: Tools->Set simulation type") {
    REQUIRE(actionSimTypeDUNE->isChecked() == true);
    REQUIRE(actionSimTypePixel->isChecked() == false);
    sendKeyEvents(&w, {"Alt+T"});
    sendKeyEvents(menu_Tools, {"S"});
    sendKeyEvents(menuSimulation_type, {"P"});
    REQUIRE(actionSimTypeDUNE->isChecked() == false);
    REQUIRE(actionSimTypePixel->isChecked() == true);
    sendKeyEvents(&w, {"Alt+T"});
    sendKeyEvents(menu_Tools, {"S"});
    sendKeyEvents(menuSimulation_type, {"D"});
    REQUIRE(actionSimTypeDUNE->isChecked() == true);
    REQUIRE(actionSimTypePixel->isChecked() == false);
  }
}

TEST_CASE("MainWindow: advanced menu", tags) {
  MainWindow w;
  auto *menu_Advanced{w.findChild<QMenu *>("menu_Advanced")};
  REQUIRE(menu_Advanced != nullptr);
  w.show();
  waitFor(&w);
  ModalWidgetTimer mwt;
  SECTION("menu: Advanced->Simulation options... ") {
    mwt.addUserAction({"Esc"});
    mwt.start();
    sendKeyEvents(&w, {"Alt+A"});
    sendKeyEvents(menu_Advanced, {"S"});
    REQUIRE(mwt.getResult() == "Simulation Options");
  }
  SECTION("menu: Advanced->Meshing options... ") {
    mwt.addUserAction({"Down", "Enter"});
    mwt.start();
    sendKeyEvents(&w, {"Alt+A"});
    sendKeyEvents(menu_Advanced, {"M"});
    REQUIRE(mwt.getResult() == "Meshing Options");
  }
}

TEST_CASE("MainWindow: geometry", tags) {
  SECTION("built-in SBML model, import built-in geometry image") {
    MainWindow w;
    w.show();
    waitFor(&w);
    openBuiltInModel(w);
    ModalWidgetTimer mwt;
    mwt.addUserAction({"Enter"});
    mwt.start();
    auto *menuImport{w.findChild<QMenu *>("menuImport")};
    REQUIRE(menuImport != nullptr);
    auto *menuExample_geometry_image{
        w.findChild<QMenu *>("menuExample_geometry_image")};
    REQUIRE(menuExample_geometry_image != nullptr);
    sendKeyEvents(&w, {"Alt+I"});
    sendKeyEvents(menuImport, {"E"});
    mwt.start();
    sendKeyEvents(menuExample_geometry_image, {"Enter"});
    REQUIRE(mwt.getResult() == "Edit Geometry Image");
  }
  SECTION("import geometry from model") {
    MainWindow w;
    w.show();
    waitFor(&w);
    ModalWidgetTimer mwt;
    SECTION("sampled field model") {
      createBinaryFile("models/brusselator-model_v3.xml", "x.xml");
      mwt.addUserAction({"x", ".", "x", "m", "l"});
      mwt.start();
      sendKeyEvents(&w, {"Ctrl+G"});
      REQUIRE(mwt.getResult() == "QFileDialog::AcceptOpen");
      QFile::remove("x.xml");
    }
    SECTION("analytic model") {
      createBinaryFile("models/analytic_2d.xml", "x.xml");
      ModalWidgetTimer analyticImportDialog;
      analyticImportDialog.addUserAction({"Enter"});
      mwt.addUserAction({"x", ".", "x", "m", "l"}, true, &analyticImportDialog);
      mwt.start();
      sendKeyEvents(&w, {"Ctrl+G"});
      REQUIRE(mwt.getResult() == "QFileDialog::AcceptOpen");
      REQUIRE(analyticImportDialog.getResult() == "Import Analytic Geometry");
      QFile::remove("x.xml");
    }
  }
  SECTION("import geometry from gmsh") {
    MainWindow w;
    w.show();
    waitFor(&w);
    openBuiltInModel(w);
    ModalWidgetTimer mwt;
    mwt.addUserAction({"Esc"});
    mwt.start();
    sendKeyEvents(&w, {"Ctrl+Shift+G"});
    REQUIRE(mwt.getResult() == "Import geometry from Gmsh");
  }
  SECTION("built-in SBML model, change geometry image zoom") {
    MainWindow w;
    w.show();
    waitFor(&w);
    openBuiltInModel(w);
    auto *spinGeometryZoom{w.findChild<QSpinBox *>("spinGeometryZoom")};
    REQUIRE(spinGeometryZoom != nullptr);
    auto *lblGeometry{w.findChild<QLabelMouseTracker *>("lblGeometry")};
    REQUIRE(lblGeometry != nullptr);
    REQUIRE(spinGeometryZoom->value() == 0);
    int width0{lblGeometry->width()};
    sendKeyEvents(spinGeometryZoom, {"Up", "Up", "Up", "Up"});
    REQUIRE(spinGeometryZoom->value() == 4);
    int width4{lblGeometry->width()};
    REQUIRE(width4 > width0);
    sendKeyEvents(spinGeometryZoom, {"Down", "Down"});
    REQUIRE(spinGeometryZoom->value() == 2);
    int width2{lblGeometry->width()};
    REQUIRE(width2 < width4);
    REQUIRE(width2 > width0);
    lblGeometry->setFocus();
    sendMouseWheel(lblGeometry, +1, Qt::KeyboardModifier::ShiftModifier);
    REQUIRE(spinGeometryZoom->value() == 3);
    int width3{lblGeometry->width()};
    REQUIRE(width3 < width4);
    REQUIRE(width3 > width2);
    sendMouseWheel(lblGeometry, -1, Qt::KeyboardModifier::ShiftModifier);
    REQUIRE(spinGeometryZoom->value() == 2);
    // without shift key, mouse wheel event is ignored
    sendMouseWheel(lblGeometry, +1);
    REQUIRE(spinGeometryZoom->value() == 2);
  }
}

TEST_CASE("MainWindow: import analytic geometry from model", tags) {
  MainWindow w;
  w.show();
  waitFor(&w);
  auto *actionGeometryFromModel{
      w.findChild<QAction *>("actionGeometry_from_model")};
  REQUIRE(actionGeometryFromModel != nullptr);
  ModalWidgetTimer mwt;
  ModalWidgetTimer analyticImportDialog;
  createBinaryFile("models/analytic_2d.xml", "x.xml");
  analyticImportDialog.addUserAction({"Enter"});
  mwt.addUserAction({"x", ".", "x", "m", "l"}, true, &analyticImportDialog);
  mwt.start();
  actionGeometryFromModel->trigger();
  REQUIRE(mwt.getResult() == "QFileDialog::AcceptOpen");
  REQUIRE(analyticImportDialog.getResult() == "Import Analytic Geometry");
  QFile::remove("x.xml");
}

TEST_CASE("MainWindow: units", tags) {
  MainWindow w;
  w.show();
  waitFor(&w);
  openBuiltInModel(w);
  auto *menu_Tools{w.findChild<QMenu *>("menu_Tools")};
  // change units
  ModalWidgetTimer mwt;
  sendKeyEvents(&w, {"Alt+T"});
  mwt.addUserAction([](QWidget *w) {
    sendKeyEvents(w->findChild<QComboBox *>("cmbTime"), {"Down"});
    sendKeyEvents(w->findChild<QComboBox *>("cmbLength"), {"Down"});
    sendKeyEvents(w->findChild<QComboBox *>("cmbVolume"), {"Down"});
    sendKeyEvents(w->findChild<QComboBox *>("cmbAmount"), {"Down"});
  });
  mwt.start();
  sendKeyEvents(menu_Tools, {"U"}, false);
  // save SBML file
  QFile::remove("tmpmainwunits.xml");
  mwt.addUserAction({"t", "m", "p", "m", "a", "i", "n", "w", "u", "n", "i", "t",
                     "s", ".", "x", "m", "l"});
  mwt.start();
  sendKeyEvents(&w, {"Ctrl+E"});
  REQUIRE(mwt.getResult() == "QFileDialog::AcceptSave");
  // check units of SBML model
  std::unique_ptr<libsbml::SBMLDocument> doc(
      libsbml::readSBMLFromFile("tmpmainwunits.xml"));
  REQUIRE(doc != nullptr);
  const auto *model = doc->getModel();
  // millisecond
  const auto *timeunit =
      model->getUnitDefinition(model->getTimeUnits())->getUnit(0);
  REQUIRE(timeunit->isSecond() == true);
  REQUIRE(timeunit->getScale() == dbl_approx(-3));
  // decimetre
  const auto *lengthunit =
      model->getUnitDefinition(model->getLengthUnits())->getUnit(0);
  REQUIRE(lengthunit->isMetre() == true);
  REQUIRE(lengthunit->getScale() == dbl_approx(-1));
  // decilitre
  const auto *volunit =
      model->getUnitDefinition(model->getVolumeUnits())->getUnit(0);
  REQUIRE(volunit->isLitre() == true);
  REQUIRE(volunit->getScale() == dbl_approx(-1));
  // millimole
  const auto *amountunit =
      model->getUnitDefinition(model->getSubstanceUnits())->getUnit(0);
  REQUIRE(amountunit->isMole() == true);
  REQUIRE(amountunit->getScale() == dbl_approx(-3));
}

TEST_CASE("MainWindow: non-spatial model import", tags) {
  ModalWidgetTimer mwt;
  auto doc{getTestSbmlDoc("non-spatial-multi-compartment")};
  libsbml::writeSBMLToFile(doc.get(), "tmpmainw-nonspatial.xml");
  // open non-spatial model
  MainWindow w("tmpmainw-nonspatial.xml");
  w.show();
  waitFor(&w);
  auto *statusBarPermanentMessage{w.statusBar()->findChild<QLabel *>()};
  REQUIRE(statusBarPermanentMessage != nullptr);
  REQUIRE(statusBarPermanentMessage->text().contains(
      "Importing non-spatial model. Step 1/3"));
  // import four-compartments built-in geometry image
  auto *menuImport{w.findChild<QMenu *>("menuImport")};
  REQUIRE(menuImport != nullptr);
  auto *menuExample_geometry_image{
      w.findChild<QMenu *>("menuExample_geometry_image")};
  REQUIRE(menuExample_geometry_image != nullptr);
  sendKeyEvents(&w, {"Alt+I"});
  sendKeyEvents(menuImport, {"E"});
  mwt.start();
  sendKeyEvents(menuExample_geometry_image, {"f", "f", "Enter"});
  REQUIRE(mwt.getResult() == "Edit Geometry Image");
  REQUIRE(statusBarPermanentMessage->text().contains(
      "Importing non-spatial model. Step 2/3"));
}

TEST_CASE("MainWindow: drag & drop events", tags) {
  MainWindow w;
  w.show();
  waitFor(&w);
  ModalWidgetTimer mwt;
  mwt.addUserAction({"Esc"});
  SECTION("drop non-existent xml file") {
    mwt.start();
    sendDropEvent(&w, "dontexist.xml");
    auto msg = mwt.getResult().toStdString();
    REQUIRE_THAT(msg, ContainsSubstring("Failed to load file"));
    REQUIRE_THAT(msg, ContainsSubstring("dontexist.xml"));
  }
  SECTION("drop non-existent tiff file") {
    mwt.start();
    sendDropEvent(&w, "dontexist.tiff");
    auto msg = mwt.getResult().toStdString();
    REQUIRE_THAT(msg, ContainsSubstring("Failed to open image file"));
    REQUIRE_THAT(msg, ContainsSubstring("dontexist.tiff"));
  }
  SECTION("drop xml file") {
    auto defaultTitle = "Spatial Model Editor [untitled-model]";
    REQUIRE(w.windowTitle() == defaultTitle);
    createBinaryFile("models/txy.xml", "drag-drop.xml");
    sendDropEvent(&w, "drag-drop.xml");
    waitFor([&]() { return w.windowTitle() != defaultTitle; });
    REQUIRE(w.windowTitle().right(15) == "[drag-drop.xml]");
  }
  SECTION("drop tiff file") {
    mwt.start();
    createBinaryFile("16bit_gray.tif", "drag-drop.tif");
    sendDropEvent(&w, "drag-drop.tif");
    REQUIRE(mwt.getResult() == "Edit Geometry Image");
  }
}
