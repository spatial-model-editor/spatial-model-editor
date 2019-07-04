#include "mainwindow.hpp"

#include <QtTest>

#include "catch.hpp"
#include "qt_test_utils.hpp"
#include "sbml_test_data/very_simple_model.hpp"

#include "qlabelmousetracker.hpp"

constexpr int key_delay = 5;

SCENARIO("Shortcut keys", "[gui][mainwindow]") {
  MainWindow w;
  w.show();
  CAPTURE(QTest::qWaitForWindowExposed(&w));
  ModalWidgetCloser mwc;
  QTabWidget *tabMain = w.topLevelWidget()->findChild<QTabWidget *>("tabMain");
  REQUIRE(tabMain != nullptr);
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

  // get pointers to some Widgets from w
  QListWidget *listCompartments =
      w.topLevelWidget()->findChild<QListWidget *>("listCompartments");
  REQUIRE(listCompartments != nullptr);
  REQUIRE(listCompartments->count() == 0);
  QListWidget *listMembranes =
      w.topLevelWidget()->findChild<QListWidget *>("listMembranes");
  REQUIRE(listMembranes != nullptr);
  REQUIRE(listMembranes->count() == 0);
  QTreeWidget *listSpecies =
      w.topLevelWidget()->findChild<QTreeWidget *>("listSpecies");
  REQUIRE(listSpecies != nullptr);
  REQUIRE(listSpecies->topLevelItemCount() == 0);
  QPushButton *btnChangeSpeciesColour =
      w.topLevelWidget()->findChild<QPushButton *>("btnChangeSpeciesColour");
  REQUIRE(btnChangeSpeciesColour != nullptr);
  QLabel *lblSpeciesColour =
      w.topLevelWidget()->findChild<QLabel *>("lblSpeciesColour");
  REQUIRE(lblSpeciesColour != nullptr);
  QTreeWidget *listReactions =
      w.topLevelWidget()->findChild<QTreeWidget *>("listReactions");
  REQUIRE(listReactions != nullptr);
  REQUIRE(listReactions->topLevelItemCount() == 0);
  QListWidget *listFunctions =
      w.topLevelWidget()->findChild<QListWidget *>("listFunctions");
  REQUIRE(listFunctions != nullptr);
  REQUIRE(listFunctions->count() == 0);
  QTabWidget *tabMain = w.topLevelWidget()->findChild<QTabWidget *>("tabMain");
  REQUIRE(tabMain != nullptr);
  QLabelMouseTracker *lblGeometry =
      w.topLevelWidget()->findChild<QLabelMouseTracker *>("lblGeometry");
  REQUIRE(lblGeometry != nullptr);
  QPushButton *btnChangeCompartment =
      w.topLevelWidget()->findChild<QPushButton *>("btnChangeCompartment");
  REQUIRE(btnChangeCompartment != nullptr);
  QPushButton *btnSimulate =
      w.topLevelWidget()->findChild<QPushButton *>("btnSimulate");
  REQUIRE(btnSimulate != nullptr);

  ModalWidgetTextInput mwti;
  // open SBML file
  mwti.start("tmp.xml");
  listCompartments->setFocus();
  QApplication::setActiveWindow(&w);
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
  listCompartments->setFocus();
  QApplication::setActiveWindow(&w);
  QTest::keyClick(&w, Qt::Key_I, Qt::ControlModifier, key_delay);
  CAPTURE(QTest::qWaitFor(
      [lblGeometry]() { return lblGeometry->getImage().size() != QSize(0, 0); },
      30000));
  REQUIRE(lblGeometry->getImage().size() == img.size());

  // assign geometry
  // c1 -> col1
  listCompartments->setCurrentRow(0);
  QApplication::processEvents();
  btnChangeCompartment->click();
  QApplication::processEvents();
  // click on top-left
  QTest::mouseClick(lblGeometry, Qt::LeftButton, Qt::KeyboardModifiers(),
                    QPoint(1, 1));
  QApplication::processEvents();
  REQUIRE(lblGeometry->getColour() == col1);
  // c2 -> col2
  listCompartments->setCurrentRow(1);
  QApplication::processEvents();
  btnChangeCompartment->click();
  QApplication::processEvents();
  // click on middle
  QTest::mouseClick(
      lblGeometry, Qt::LeftButton, Qt::KeyboardModifiers(),
      QPoint(lblGeometry->width() - 1, lblGeometry->height() - 1));
  QApplication::processEvents();
  REQUIRE(lblGeometry->getColour() == col2);
  // c3 -> col3
  listCompartments->setCurrentRow(2);
  QApplication::processEvents();
  btnChangeCompartment->click();
  QApplication::processEvents();
  // click on bottom right
  QTest::mouseClick(
      lblGeometry, Qt::LeftButton, Qt::KeyboardModifiers(),
      QPoint(lblGeometry->width() / 2, lblGeometry->height() / 2));
  QApplication::processEvents();
  REQUIRE(lblGeometry->getColour() == col3);
  // if lblGeometry has focus then ctrl+tab doesn't work to change tabs:
  listCompartments->setCurrentRow(0);
  QApplication::processEvents();
  listCompartments->setFocus();
  QApplication::processEvents();
  // display membrane tab
  QTest::keyPress(w.windowHandle(), Qt::Key_Tab, Qt::ControlModifier,
                  key_delay);
  CAPTURE(
      QTest::qWaitFor([tabMain]() { return tabMain->currentIndex() == 1; }));
  REQUIRE(listMembranes->count() == 3);
  // select each item in listMembranes
  listMembranes->setFocus();
  if (listMembranes->count() > 0) {
    listMembranes->setCurrentRow(0);
  }
  for (int i = 0; i < listMembranes->count(); ++i) {
    QTest::keyClick(listMembranes, Qt::Key_Down, Qt::ControlModifier,
                    key_delay);
    QTest::keyClick(listMembranes, Qt::Key_Enter, Qt::ControlModifier,
                    key_delay);
  }

  // display species tab
  QTest::keyPress(w.windowHandle(), Qt::Key_Tab, Qt::ControlModifier,
                  key_delay);
  QApplication::processEvents();
  REQUIRE(listSpecies->topLevelItemCount() == 3);
  // select each item in listSpecies
  listSpecies->setFocus();
  listSpecies->setCurrentItem(listSpecies->topLevelItem(0));
  for (int i = 0; i < 8; ++i) {
    QTest::keyClick(listSpecies, Qt::Key_Down, Qt::ControlModifier, key_delay);
    QTest::keyClick(listSpecies, Qt::Key_Enter, Qt::ControlModifier, key_delay);
  }
  REQUIRE(lblSpeciesColour->pixmap()->toImage().pixelColor(0, 0) ==
          sbml::defaultSpeciesColours()[3]);
  // just click Enter, so accept default colour, ie no-op
  mwti.start("");
  QTest::mouseClick(btnChangeSpeciesColour, Qt::LeftButton,
                    Qt::KeyboardModifier(), QPoint(), key_delay);
  REQUIRE(lblSpeciesColour->pixmap()->toImage().pixelColor(0, 0) ==
          sbml::defaultSpeciesColours()[3]);

  // display reactions tab
  QTest::keyPress(w.windowHandle(), Qt::Key_Tab, Qt::ControlModifier,
                  key_delay);
  QApplication::processEvents();
  REQUIRE(listReactions->topLevelItemCount() == 3);
  // select each item in listReactions
  listReactions->setFocus();
  listReactions->setCurrentItem(listReactions->topLevelItem(0));
  for (int i = 0; i < 7; ++i) {
    QTest::keyClick(listReactions, Qt::Key_Down, Qt::ControlModifier,
                    key_delay);
    QTest::keyClick(listReactions, Qt::Key_Enter, Qt::ControlModifier,
                    key_delay);
  }

  // display functions tab
  QTest::keyPress(w.windowHandle(), Qt::Key_Tab, Qt::ControlModifier,
                  key_delay);
  QApplication::processEvents();
  REQUIRE(listFunctions->count() == 0);
  // select each item in listSpecies
  listFunctions->setFocus();
  if (listFunctions->count() > 0) {
    listFunctions->setCurrentRow(0);
  }
  for (int i = 0; i < listFunctions->count(); ++i) {
    QTest::keyClick(listFunctions, Qt::Key_Down, Qt::ControlModifier,
                    key_delay);
    QTest::keyClick(listFunctions, Qt::Key_Enter, Qt::ControlModifier,
                    key_delay);
  }

  // display simulate tab
  QApplication::processEvents();
  QTest::keyPress(w.windowHandle(), Qt::Key_Tab, Qt::ControlModifier,
                  key_delay);
  btnSimulate->click();
}
