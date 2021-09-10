#include "catch_wrapper.hpp"
#include "mainwindow.hpp"
#include "qlabelmousetracker.hpp"
#include "qt_test_utils.hpp"
#include <QFile>
#include <QMenu>
#include <QSpinBox>
#include <sbml/SBMLTypes.h>
#include <sbml/extension/SBMLDocumentPlugin.h>
#include <sbml/packages/spatial/common/SpatialExtensionTypes.h>
#include <sbml/packages/spatial/extension/SpatialExtension.h>

using namespace sme::test;

static void openBuiltInModel(MainWindow &w, const QString &shortcutKey = "V") {
  auto *menuFile = w.findChild<QMenu *>("menuFile");
  auto *menuOpen_example_SBML_file{
      w.findChild<QMenu *>("menuOpen_example_SBML_file")};
  sendKeyEvents(&w, {"Alt+F"});
  sendKeyEvents(menuFile, {"E"});
  sendKeyEvents(menuOpen_example_SBML_file, {shortcutKey});
}

TEST_CASE("Mainwindow", "[gui/mainwindow][gui][mainwindow]") {
  SECTION("non-existent file") {
    ModalWidgetTimer mwt;
    mwt.addUserAction({"Esc"});
    mwt.start();
    MainWindow w("dontexist.xml");
    w.show();
    REQUIRE(mwt.getResult() ==
            "Failed to load file dontexist.xml\n\n[2] "
            "[Operating system] line 1:1 File unreadable.\n\n");
  }
  SECTION("shortcut keys") {
    MainWindow w;
    auto *menu_Tools{w.findChild<QMenu *>("menu_Tools")};
    REQUIRE(menu_Tools != nullptr);
    auto *menuSimulation_type{w.findChild<QMenu *>("menuSimulation_type")};
    REQUIRE(menuSimulation_type != nullptr);
    auto *actionSimTypeDUNE{w.findChild<QAction *>("actionSimTypeDUNE")};
    REQUIRE(actionSimTypeDUNE != nullptr);
    auto *actionSimTypePixel{w.findChild<QAction *>("actionSimTypePixel")};
    REQUIRE(actionSimTypePixel != nullptr);
    auto *menu_Advanced{w.findChild<QMenu *>("menu_Advanced")};
    REQUIRE(menu_Advanced != nullptr);
    auto *tabMain{w.findChild<QTabWidget *>("tabMain")};
    REQUIRE(tabMain != nullptr);
    w.show();
    waitFor(&w);
    ModalWidgetTimer mwt;
    SECTION("F8") {
      mwt.start();
      sendKeyEvents(&w, {"F8"});
      REQUIRE(mwt.getResult() == "About Spatial Model Editor");
    }
    SECTION("F9") {
      mwt.start();
      sendKeyEvents(&w, {"F9"});
      QString correctText = "<h3>About Qt</h3>";
      CAPTURE(mwt.getResult());
      REQUIRE(mwt.getResult().left(correctText.size()) == correctText);
    }
    SECTION("Ctrl+N") {
      // ctrl+n to create new model, then escape to cancel
      QString oldTitle{w.windowTitle()};
      mwt.addUserAction({"Esc"}, false);
      mwt.start();
      sendKeyEvents(&w, {"Ctrl+N"});
      REQUIRE(mwt.getResult() == "Create new model");
      REQUIRE(w.windowTitle() == oldTitle);
      // ctrl+n to create new model with name "new"
      mwt.addUserAction({"n", "e", "w"});
      mwt.start();
      sendKeyEvents(&w, {"Ctrl+N"});
      REQUIRE(mwt.getResult() == "Create new model");
      REQUIRE(w.windowTitle().right(9) == "[new.sme]");
    }
    SECTION("user presses ctrl+s (with valid SBML model)") {
      openBuiltInModel(w, "V");
      SECTION("cancel") {
        mwt.addUserAction(QStringList{"Escape"}, false);
        mwt.start();
        sendKeyEvents(&w, {"Ctrl+S"});
        REQUIRE(mwt.getResult() == "QFileDialog::AcceptSave");
      }
      SECTION("save sme file") {
        QFile::remove("wqz.sme");
        mwt.addUserAction({"w", "q", "z"});
        mwt.start();
        sendKeyEvents(&w, {"Ctrl+S"});
        REQUIRE(mwt.getResult() == "QFileDialog::AcceptSave");
      }
    }
    SECTION("user presses ctrl+e (with valid SBML model)") {
      openBuiltInModel(w, "V");
      SECTION("cancel") {
        mwt.addUserAction(QStringList{"Escape"}, false);
        mwt.start();
        sendKeyEvents(&w, {"Ctrl+E"});
        REQUIRE(mwt.getResult() == "QFileDialog::AcceptSave");
      }
      SECTION("save sbml xml file") {
        QFile::remove("wqz.xml");
        mwt.addUserAction({"w", "q", "z"});
        mwt.start();
        sendKeyEvents(&w, {"Ctrl+E"});
        REQUIRE(mwt.getResult() == "QFileDialog::AcceptSave");
        QFile file("wqz.xml");
        REQUIRE(file.open(QIODevice::ReadOnly | QIODevice::Text));
        auto line = file.readLine().toStdString();
        REQUIRE(line == "<?xml version=\"1.0\" encoding=\"UTF-8\"?>\n");
      }
    }
    SECTION("user presses ctrl+d (SBML model but no geometry loaded)") {
      SECTION("offer to import a geometry image") {
        mwt.addUserAction({"Esc"});
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
        mwt.addUserAction(QStringList{"Escape"});
        mwt.start();
        sendKeyEvents(&w, {"Ctrl+D"});
        REQUIRE(mwt.getResult() == "QFileDialog::AcceptSave");
      }
      SECTION("save dune ini file") {
        openBuiltInModel(w, "A");
        mwt.addUserAction({"w", "q", "z"});
        mwt.start();
        sendKeyEvents(&w, {"Ctrl+D"});
        REQUIRE(mwt.getResult() == "QFileDialog::AcceptSave");
        QFile file("wqz_comp.ini");
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
        mwt.addUserAction({"w", "q", "z", ".", "x", "m", "l"});
        mwt.start();
        sendKeyEvents(&w, {"Ctrl+O"});
        REQUIRE(mwt.getResult() == "QFileDialog::AcceptOpen");
        REQUIRE(w.windowTitle().right(8) == "wqz.xml]");
      }
      SECTION("open sme file") {
        REQUIRE(w.windowTitle() == "Spatial Model Editor [untitled-model]");
        mwt.addUserAction({"w", "q", "z", ".", "s", "m", "e"});
        mwt.start();
        sendKeyEvents(&w, {"Ctrl+O"});
        REQUIRE(mwt.getResult() == "QFileDialog::AcceptOpen");
        REQUIRE(w.windowTitle().right(8) == "wqz.sme]");
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
    SECTION("menu: Advanced->Simulation options... ") {
      mwt.addUserAction({"Esc"});
      mwt.start();
      sendKeyEvents(&w, {"Alt+A"});
      sendKeyEvents(menu_Advanced, {"S"});
      REQUIRE(mwt.getResult() == "Simulation Options");
    }
  }
  SECTION("built-in SBML model, import built-in geometry image") {
    MainWindow w;
    w.show();
    waitFor(&w);
    openBuiltInModel(w);
    ModalWidgetTimer mwt;
    mwt.addUserAction({"Enter"});
    mwt.start();
    auto *menu_Import{w.findChild<QMenu *>("menu_Tools")};
    REQUIRE(menu_Import != nullptr);
    sendKeyEvents(&w, {"Alt+I"});
    sendKeyEvents(menu_Import, {"E"});
    sendKeyEvents(&w, {"Enter"});
    REQUIRE(mwt.getResult() == "Edit Geometry Image");
  }
  SECTION("import geometry from model") {
    MainWindow w;
    w.show();
    waitFor(&w);
    ModalWidgetTimer mwt;
    mwt.addUserAction({"w", "q", "z", ".", "x", "m", "l"});
    mwt.start();
    auto *menu_Import{w.findChild<QMenu *>("menu_Tools")};
    REQUIRE(menu_Import != nullptr);
    sendKeyEvents(&w, {"Ctrl+G"});
    REQUIRE(mwt.getResult() == "QFileDialog::AcceptOpen");
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
  SECTION("built-in SBML model, change units") {
    MainWindow w;
    w.show();
    waitFor(&w);
    openBuiltInModel(w);
    auto *menu_Tools = w.findChild<QMenu *>("menu_Tools");
    // change units
    ModalWidgetTimer mwt;
    sendKeyEvents(&w, {"Alt+T"});
    mwt.addUserAction({"Down", "Tab", "Down", "Tab", "Down", "Tab", "Down"});
    mwt.start();
    sendKeyEvents(menu_Tools, {"U"}, false);
    // save SBML file
    QFile::remove("units.xml");
    mwt.addUserAction({"u", "n", "i", "t", "s", ".", "x", "m", "l"});
    mwt.start();
    sendKeyEvents(&w, {"Ctrl+E"});
    // check units of SBML model
    std::unique_ptr<libsbml::SBMLDocument> doc(
        libsbml::readSBMLFromFile("units.xml"));
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
}
