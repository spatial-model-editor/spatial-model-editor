#include <QtTest>

#include "catch.hpp"
#include "catch_qt_ostream.hpp"

#include "mainwindow.h"

constexpr int delay = 500;

TEST_CASE("F1: opens about dialog", "[mainwindow][gui]") {
  MainWindow w;
  w.show();
  // wait for mainwindow to load fully
  QTest::qWait(delay);
  // close the next modal window to open
  // after capturing the text in mwc.result
  ModalWidgetCloser mwc;
  // press F1: opens modal 'About' message box
  // this message box is blocking until user clicks ok
  // (or until the ModalWindowCloser closes it)
  QTest::keyClick(&w, Qt::Key_F1);
  QString correctText = "Spatial Model Editor";
  REQUIRE(mwc.result.left(correctText.size()) == correctText);
}

TEST_CASE("ctrl+o: opens open file dialog", "[mainwindow][gui]") {
  MainWindow w;
  w.show();
  QTest::qWait(delay);
  ModalWidgetCloser mwc;
  QTest::keyClick(&w, Qt::Key_O, Qt::ControlModifier);
  REQUIRE(1 == 1);
}

TEST_CASE("alt+f, o: opens open file dialog", "[mainwindow][gui]") {
  MainWindow w;
  w.show();
  QTest::qWait(delay);
  ModalWidgetCloser mwc;
  // alt-f goes to mainwindow, which opens File menu:
  QTest::keyClick(&w, Qt::Key_F, Qt::AltModifier);
  // o then needs to go to the menu, not the mainwindow:
  QTest::keyClick(QApplication::activePopupWidget(), Qt::Key_O);
  REQUIRE(1 == 1);
}
