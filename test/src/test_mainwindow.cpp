#include <QtTest>
#include "catch_qt.h"
#include "qt_test_utils.h"
#include "sbml_test_data/very_simple_model.h"

#include "mainwindow.h"

constexpr int key_delay = 50;

SCENARIO("Shortcut keys", "[gui][mainwindow]") {
  MainWindow w;
  w.show();
  CAPTURE(QTest::qWaitForWindowExposed(&w));
  WHEN("user presses F1") {
    THEN("open About dialog box") {
      // this object closes the next modal window to open
      // after capturing the text in mwc.result
      ModalWidgetCloser mwc;
      // press F1: opens modal 'About' message box
      // this message box is blocking until user clicks ok
      // (or until the ModalWindowCloser closes it)
      QTest::keyClick(&w, Qt::Key_F1);
      QString correctText = "Spatial Model Editor";
      REQUIRE(mwc.result.left(correctText.size()) == correctText);
    }
  }
  WHEN("user presses ctrl+o") {
    THEN("open AcceptOpen FileDialog") {
      ModalWidgetCloser mwc;
      QTest::keyClick(&w, Qt::Key_O, Qt::ControlModifier);
      REQUIRE(mwc.result == "QFileDialog::AcceptOpen");
    }
  }
  WHEN("user presses ctrl+s") {
    THEN("open AcceptSave FileDialog") {
      ModalWidgetCloser mwc;
      QTest::keyClick(&w, Qt::Key_S, Qt::ControlModifier);
      REQUIRE(mwc.result == "QFileDialog::AcceptSave");
    }
  }
  WHEN("user presses ctrl+tab") {
    THEN("display next tab") {
      int nTabs = 7;
      QTabWidget *tabMain =
          w.topLevelWidget()->findChild<QTabWidget *>("tabMain");
      REQUIRE(tabMain != nullptr);
      REQUIRE(tabMain->currentIndex() == 0);
      for (int index = 1; index < 2 * nTabs + 1; ++index) {
        QTest::keyPress(w.windowHandle(), Qt::Key_Tab, Qt::ControlModifier,
                        key_delay);
        REQUIRE(tabMain->currentIndex() == index % nTabs);
      }
    }
  }
}

SCENARIO("Load SBML file", "[gui][mainwindow]") {
  // create SBML file to load
  std::unique_ptr<libsbml::SBMLDocument> doc(
      libsbml::readSBMLFromString(sbml_test_data::very_simple_model().xml));
  // write SBML document to file
  libsbml::SBMLWriter().writeSBML(doc.get(), "tmp.xml");

  MainWindow w;
  w.show();
  CAPTURE(QTest::qWaitForWindowExposed(&w));
  QListWidget *listCompartments =
      w.topLevelWidget()->findChild<QListWidget *>("listCompartments");
  REQUIRE(listCompartments != nullptr);
  REQUIRE(listCompartments->count() == 0);

  // open SBML file
  ModalWidgetTextInput mwti("tmp.xml");
  QTest::keyClick(&w, Qt::Key_O, Qt::ControlModifier, key_delay);
  REQUIRE(listCompartments->count() == 3);

  // import Geometry
}
