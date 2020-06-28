#include <sbml/SBMLTypes.h>
#include <sbml/extension/SBMLDocumentPlugin.h>
#include <sbml/packages/spatial/common/SpatialExtensionTypes.h>
#include <sbml/packages/spatial/extension/SpatialExtension.h>

#include <QFile>
#include <QMenu>

#include "catch_wrapper.hpp"
#include "mainwindow.hpp"
#include "qt_test_utils.hpp"

static void openBuiltInModel(MainWindow &w, const QString &shortcutKey = "V") {
  auto *menuFile = w.findChild<QMenu *>("menuFile");
  auto *menuOpen_example_SBML_file =
      w.findChild<QMenu *>("menuOpen_example_SBML_file");
  sendKeyEvents(&w, {"Alt+F"});
  sendKeyEvents(menuFile, {"E"});
  sendKeyEvents(menuOpen_example_SBML_file, {shortcutKey});
}

SCENARIO("Mainwindow: ", "[gui/mainwindow][gui][mainwindow]") {
  GIVEN("open non-existent file") {
    ModalWidgetTimer mwt;
    mwt.addUserAction({"Esc"});
    mwt.start();
    MainWindow w("dontexist.xml");
    w.show();
  }
  GIVEN("shortcut keys") {
    MainWindow w;
    auto *menu_Tools = w.findChild<QMenu *>("menu_Tools");
    auto *menuSimulation_type = w.findChild<QMenu *>("menuSimulation_type");
    auto *actionSimTypeDUNE = w.findChild<QAction *>("actionSimTypeDUNE");
    auto *actionSimTypePixel = w.findChild<QAction *>("actionSimTypePixel");
    auto *menu_Advanced = w.findChild<QMenu *>("menu_Advanced");
    auto *tabMain = w.findChild<QTabWidget *>("tabMain");
    w.show();
    waitFor(&w);
    ModalWidgetTimer mwt;
    WHEN("user presses F8") {
      THEN("open About dialog box") {
        mwt.start();
        sendKeyEvents(&w, {"F8"});
      }
    }
    WHEN("user presses F9") {
      THEN("open About Qt dialog box") {
        mwt.start();
        sendKeyEvents(&w, {"F9"});
        QString correctText = "<h3>About Qt</h3>";
        CAPTURE(mwt.getResult());
        REQUIRE(mwt.getResult().left(correctText.size()) == correctText);
      }
    }
    WHEN("user presses ctrl+n") {
      THEN("ask for name of new model") {
        mwt.addUserAction({"Esc"});
        mwt.start();
        sendKeyEvents(&w, {"Ctrl+N"});
        REQUIRE(w.windowTitle().right(20) == "[untitled-model.xml]");
        mwt.addUserAction({"n", "e", "w"});
        mwt.start();
        sendKeyEvents(&w, {"Ctrl+N"});
        REQUIRE(w.windowTitle().right(9) == "[new.xml]");
      }
    }
    WHEN("user presses ctrl+o") {
      THEN("open AcceptOpen FileDialog") {
        mwt.addUserAction({"Esc"});
        mwt.start();
        sendKeyEvents(&w, {"Ctrl+O"});
        REQUIRE(mwt.getResult() == "QFileDialog::AcceptOpen");
      }
    }
    WHEN("user presses ctrl+s (SBML model but no geometry loaded)") {
      THEN("offer to import a geometry image") {
        sendKeyEvents(&w, {"Ctrl+S"});
        // press no when asked to import image
        auto title = sendKeyEventsToNextQDialog({"Esc"});
        REQUIRE(title == "No compartment geometry image");
      }
    }
    WHEN("user presses ctrl+s (with valid SBML model)") {
      openBuiltInModel(w, "V");
      THEN("cancel") {
        mwt.addUserAction(QStringList{"Escape"});
        mwt.start();
        sendKeyEvents(&w, {"Ctrl+S"});
        REQUIRE(mwt.getResult() == "QFileDialog::AcceptSave");
      }
      THEN("save xml file") {
        mwt.addUserAction({"w", "q", "z"});
        mwt.start();
        sendKeyEvents(&w, {"Ctrl+S"});
        REQUIRE(mwt.getResult() == "QFileDialog::AcceptSave");
        QFile file("wqz.xml");
        REQUIRE(file.open(QIODevice::ReadOnly | QIODevice::Text));
        auto line = file.readLine().toStdString();
        REQUIRE(line == "<?xml version=\"1.0\" encoding=\"UTF-8\"?>\n");
      }
    }
    WHEN("user presses ctrl+d (SBML model but no geometry loaded)") {
      THEN("offer to import a geometry image") {
        sendKeyEvents(&w, {"Ctrl+D"});
        // press no when asked to import image
        auto title = sendKeyEventsToNextQDialog({"Esc"});
        REQUIRE(title == "No compartment geometry image");
      }
    }
    WHEN("user presses ctrl+d (with valid SBML model)") {
      THEN("cancel") {
        openBuiltInModel(w, "A");
        mwt.addUserAction(QStringList{"Escape"});
        mwt.start();
        sendKeyEvents(&w, {"Ctrl+D"});
        REQUIRE(mwt.getResult() == "QFileDialog::AcceptSave");
      }
      THEN("save dune ini file") {
        openBuiltInModel(w, "A");
        mwt.addUserAction({"w", "q", "z"});
        mwt.start();
        sendKeyEvents(&w, {"Ctrl+D"});
        REQUIRE(mwt.getResult() == "QFileDialog::AcceptSave");
        QFile file("wqz.ini");
        REQUIRE(file.open(QIODevice::ReadOnly | QIODevice::Text));
        auto line = file.readLine().toStdString();
        REQUIRE(line == "[grid]\n");
      }
    }
    WHEN("user presses ctrl+I to import image (default empty SBML model)") {
      THEN("offer to import SBML model") {
        // escape on SBML file open dialog
        mwt.addUserAction({"Esc"}, false);
        mwt.start();
        sendKeyEvents(&w, {"Ctrl+I"});
        REQUIRE(mwt.getResult() == "QFileDialog::AcceptOpen");
      }
    }
    WHEN("user presses ctrl+tab (default SBML model loaded)") {
      THEN("remain on Geometry tab: all others disabled") {
        REQUIRE(tabMain->currentIndex() == 0);
        sendKeyEvents(tabMain, {"Ctrl+Tab"});
        REQUIRE(tabMain->currentIndex() == 0);
      }
    }
    WHEN("user presses ctrl+shift+tab (default SBML model loaded)") {
      THEN("remain on Geometry tab: all others disabled") {
        REQUIRE(tabMain->currentIndex() == 0);
        sendKeyEvents(tabMain, {"Ctrl+Shift+Tab"});
        REQUIRE(tabMain->currentIndex() == 0);
      }
    }
    WHEN("user presses ctrl+tab with valid model loaded") {
      THEN("cycle through tabs") {
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
        REQUIRE(tabMain->currentIndex() == 7);
        sendKeyEvents(tabMain, {"Ctrl+Tab"});
        REQUIRE(tabMain->currentIndex() == 8);
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
        REQUIRE(tabMain->currentIndex() == 8);
      }
    }
    WHEN("menu: Tools->Set model units (default SBML model)") {
      THEN("offer to import SBML model") {
        mwt.addUserAction({"Esc"}, false);
        mwt.start();
        sendKeyEvents(&w, {"Alt+T"});
        sendKeyEvents(menu_Tools, {"U"});
        // press esc to close dialog
        REQUIRE(mwt.getResult() == "");
      }
    }
    WHEN("menu: Tools->Set image size (default SBML model, no image)") {
      THEN("offer to import geometry image") {
        sendKeyEvents(&w, {"Alt+T"});
        sendKeyEvents(menu_Tools, {"I"});
        // press no when asked to import image
        auto title = sendKeyEventsToNextQDialog({"Esc"});
        REQUIRE(title == "No compartment geometry image");
      }
    }
    WHEN("menu: Tools->Set simulation type") {
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
    WHEN("menu: Advanced->Max cpu threads... ") {
      THEN("cancel") {
        mwt.addUserAction({"Esc"});
        mwt.start();
        sendKeyEvents(&w, {"Alt+A "});
        sendKeyEvents(menu_Advanced, {"M"});
        REQUIRE(mwt.getResult() == "");
      }
      THEN("enter a value") {
        mwt.addUserAction({"7"});
        mwt.start();
        sendKeyEvents(&w, {"Alt+A "});
        sendKeyEvents(menu_Advanced, {"M"});
        REQUIRE(mwt.getResult() == "");
      }
    }
  }
  GIVEN("built-in SBML model, change units") {
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
    sendKeyEvents(&w, {"Ctrl+S"});
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
