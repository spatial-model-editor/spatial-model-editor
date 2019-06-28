#include <QtTest>
#include "catch_qt.h"
#include "qt_test_utils.h"

#include "mainwindow.h"

constexpr int delay = 250;

constexpr int key_delay = 50;

SCENARIO("Shortcut keys", "[gui][mainwindow]") {
  MainWindow w;
  w.show();
  // wait for mainwindow to load fully
  QTest::qWait(delay);
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
    THEN("open FileDialog") {
      ModalWidgetCloser mwc;
      QTest::keyClick(&w, Qt::Key_O, Qt::ControlModifier);
      REQUIRE(mwc.result == "QFileDialog::AcceptOpen");
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
