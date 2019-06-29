#include <QtTest>
#include "catch_qt.h"
#include "mainwindow.h"
#include "qlabelmousetracker.h"
#include "qt_test_utils.h"
#include "sbml_test_data/very_simple_model.h"

constexpr int key_delay = 50;

SCENARIO("Shortcut keys", "[gui][mainwindow]") {
  MainWindow w;
  w.show();
  CAPTURE(QTest::qWaitForWindowExposed(&w));
  ModalWidgetCloser mwc;
  WHEN("user presses F1") {
    THEN("open About dialog box") {
      // this object closes the next modal window to open
      // after capturing the text in mwc.result
      mwc.start();
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
      mwc.start();
      QTest::keyClick(&w, Qt::Key_O, Qt::ControlModifier);
      REQUIRE(mwc.result == "QFileDialog::AcceptOpen");
    }
  }
  WHEN("user presses ctrl+s") {
    THEN("open AcceptSave FileDialog") {
      mwc.start();
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
  WHEN("user presses ctrl+shift+tab") {
    THEN("display previous tab") {
      int nTabs = 7;
      QTabWidget *tabMain =
          w.topLevelWidget()->findChild<QTabWidget *>("tabMain");
      REQUIRE(tabMain != nullptr);
      REQUIRE(tabMain->currentIndex() == 0);
      for (int index = -1; index > -2 * nTabs + 1; --index) {
        QTest::keyPress(w.windowHandle(), Qt::Key_Tab,
                        Qt::ControlModifier | Qt::ShiftModifier, key_delay);
        REQUIRE(tabMain->currentIndex() == (index + 5 * nTabs) % nTabs);
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
  ModalWidgetTextInput mwti;
  mwti.start("tmp.xml");
  QTest::keyClick(&w, Qt::Key_O, Qt::ControlModifier, key_delay);
  REQUIRE(listCompartments->count() == 3);

  // create geometry image
  QImage img(3, 3, QImage::Format_RGB32);
  QRgb col1 = QColor(112, 43, 4).rgba();
  QRgb col2 = QColor(92, 142, 22).rgba();
  QRgb col3 = QColor(33, 77, 221).rgba();
  CAPTURE(col1);
  CAPTURE(col2);
  CAPTURE(col3);
  img.fill(col1);
  img.setPixel(2, 0, col2);
  img.setPixel(2, 1, col2);
  img.setPixel(2, 2, col2);
  img.setPixel(1, 1, col3);
  img.save("tmp.bmp");

  // import Geometry from image
  mwti.start("tmp.bmp");
  QTest::keyClick(&w, Qt::Key_I, Qt::ControlModifier, key_delay);
  QLabelMouseTracker *lblGeometry =
      w.topLevelWidget()->findChild<QLabelMouseTracker *>("lblGeometry");
  // wait until image is being displayed by QLabelMouseTracker
  CAPTURE(QTest::qWaitFor([lblGeometry]() {
    return lblGeometry->getImage().size() != QSize(0, 0);
  }));
  REQUIRE(lblGeometry != nullptr);
  REQUIRE(lblGeometry->getImage().size() == img.size());

  // assign geometry
  QPushButton *btnChangeCompartment =
      w.topLevelWidget()->findChild<QPushButton *>("btnChangeCompartment");
  // c1 -> col1
  listCompartments->setCurrentRow(0);
  QApplication::processEvents();
  btnChangeCompartment->click();
  QApplication::processEvents();
  QTest::mouseClick(lblGeometry, Qt::LeftButton, Qt::KeyboardModifiers(),
                    QPoint(1, 1), 0);
  QApplication::processEvents();
  REQUIRE(lblGeometry->getColour() == col1);
  // c2 -> col2
  listCompartments->setCurrentRow(1);
  QApplication::processEvents();
  btnChangeCompartment->click();
  QApplication::processEvents();
  QTest::mouseClick(lblGeometry, Qt::LeftButton, Qt::KeyboardModifiers(),
                    QPoint(lblGeometry->width() - 1, lblGeometry->height() - 1),
                    0);
  QApplication::processEvents();
  REQUIRE(lblGeometry->getColour() == col2);
  // c3 -> col3
  listCompartments->setCurrentRow(2);
  QApplication::processEvents();
  btnChangeCompartment->click();
  QApplication::processEvents();
  QTest::mouseClick(lblGeometry, Qt::LeftButton, Qt::KeyboardModifiers(),
                    QPoint(lblGeometry->width() / 2, lblGeometry->height() / 2),
                    0);
  QApplication::processEvents();
  REQUIRE(lblGeometry->getColour() == col3);
}
