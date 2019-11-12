#include <qcustomplot.h>
#include <sbml/SBMLTypes.h>

#include <QtTest>

#include "catch_wrapper.hpp"
#include "logger.hpp"
#include "mainwindow.hpp"
#include "qlabelmousetracker.hpp"
#include "qt_test_utils.hpp"
#include "sbml_test_data/ABtoC.hpp"
#include "sbml_test_data/very_simple_model.hpp"
#include "utils.hpp"

// class that provides a pointer to each Widget in mainWindow
class UIPointers {
 public:
  explicit UIPointers(MainWindow *mainWindow);
  MainWindow *w;
  QMenu *menuFile;
  QMenu *menuImport;
  QMenu *menuTools;
  QMenu *menuExample_geometry_image;
  QMenu *menuOpen_example_SBML_file;
  QLabelMouseTracker *lblGeometry;
  QLabelMouseTracker *lblCompShape;
  QLabelMouseTracker *lblCompMesh;
  QLabelMouseTracker *lblCompBoundary;
  QListWidget *listCompartments;
  QListWidget *listMembranes;
  QTreeWidget *listSpecies;

  QPushButton *btnChangeSpeciesColour;
  QCheckBox *chkSpeciesIsSpatial;
  QCheckBox *chkSpeciesIsConstant;
  QLabel *lblSpeciesColour;
  QTreeWidget *listReactions;
  QListWidget *listFunctions;
  QTabWidget *tabMain;
  QTabWidget *tabCompartmentGeometry;
  QPushButton *btnChangeCompartment;
  QLineEdit *txtSimLength;
  QLineEdit *txtSimInterval;
  QLineEdit *txtSimDt;
  QPushButton *btnSimulate;
  QPushButton *btnResetSimulation;
  QCustomPlot *pltPlot;
};

UIPointers::UIPointers(MainWindow *mainWindow) : w(mainWindow) {
  menuFile = w->topLevelWidget()->findChild<QMenu *>("menuFile");
  REQUIRE(menuFile != nullptr);
  menuImport = w->topLevelWidget()->findChild<QMenu *>("menuImport");
  REQUIRE(menuImport != nullptr);
  menuTools = w->topLevelWidget()->findChild<QMenu *>("menu_Tools");
  REQUIRE(menuTools != nullptr);
  menuExample_geometry_image =
      w->topLevelWidget()->findChild<QMenu *>("menuExample_geometry_image");
  REQUIRE(menuExample_geometry_image != nullptr);
  menuOpen_example_SBML_file =
      w->topLevelWidget()->findChild<QMenu *>("menuOpen_example_SBML_file");
  REQUIRE(menuOpen_example_SBML_file != nullptr);
  lblGeometry =
      w->topLevelWidget()->findChild<QLabelMouseTracker *>("lblGeometry");
  REQUIRE(lblGeometry != nullptr);
  lblCompShape =
      w->topLevelWidget()->findChild<QLabelMouseTracker *>("lblCompShape");
  REQUIRE(lblCompShape != nullptr);
  lblCompMesh =
      w->topLevelWidget()->findChild<QLabelMouseTracker *>("lblCompMesh");
  REQUIRE(lblCompMesh != nullptr);
  lblCompBoundary =
      w->topLevelWidget()->findChild<QLabelMouseTracker *>("lblCompBoundary");
  REQUIRE(lblCompBoundary != nullptr);
  listCompartments =
      w->topLevelWidget()->findChild<QListWidget *>("listCompartments");
  REQUIRE(listCompartments != nullptr);
  listMembranes =
      w->topLevelWidget()->findChild<QListWidget *>("listMembranes");
  REQUIRE(listMembranes != nullptr);
  listSpecies = w->topLevelWidget()->findChild<QTreeWidget *>("listSpecies");
  REQUIRE(listSpecies != nullptr);
  btnChangeSpeciesColour =
      w->topLevelWidget()->findChild<QPushButton *>("btnChangeSpeciesColour");
  REQUIRE(btnChangeSpeciesColour != nullptr);
  chkSpeciesIsSpatial =
      w->topLevelWidget()->findChild<QCheckBox *>("chkSpeciesIsSpatial");
  REQUIRE(chkSpeciesIsSpatial != nullptr);
  chkSpeciesIsConstant =
      w->topLevelWidget()->findChild<QCheckBox *>("chkSpeciesIsConstant");
  REQUIRE(chkSpeciesIsConstant != nullptr);
  lblSpeciesColour =
      w->topLevelWidget()->findChild<QLabel *>("lblSpeciesColour");
  REQUIRE(lblSpeciesColour != nullptr);
  listReactions =
      w->topLevelWidget()->findChild<QTreeWidget *>("listReactions");
  REQUIRE(listReactions != nullptr);
  listFunctions =
      w->topLevelWidget()->findChild<QListWidget *>("listFunctions");
  REQUIRE(listFunctions != nullptr);
  tabMain = w->topLevelWidget()->findChild<QTabWidget *>("tabMain");
  REQUIRE(tabMain != nullptr);
  tabCompartmentGeometry =
      w->topLevelWidget()->findChild<QTabWidget *>("tabCompartmentGeometry");
  REQUIRE(tabCompartmentGeometry != nullptr);
  btnChangeCompartment =
      w->topLevelWidget()->findChild<QPushButton *>("btnChangeCompartment");
  REQUIRE(btnChangeCompartment != nullptr);
  txtSimLength = w->topLevelWidget()->findChild<QLineEdit *>("txtSimLength");
  REQUIRE(txtSimLength != nullptr);
  txtSimInterval =
      w->topLevelWidget()->findChild<QLineEdit *>("txtSimInterval");
  REQUIRE(txtSimInterval != nullptr);
  txtSimDt = w->topLevelWidget()->findChild<QLineEdit *>("txtSimDt");
  REQUIRE(txtSimDt != nullptr);
  btnSimulate = w->topLevelWidget()->findChild<QPushButton *>("btnSimulate");
  REQUIRE(btnSimulate != nullptr);
  btnResetSimulation =
      w->topLevelWidget()->findChild<QPushButton *>("btnResetSimulation");
  REQUIRE(btnResetSimulation != nullptr);
  pltPlot = w->topLevelWidget()->findChild<QCustomPlot *>("pltPlot");
  REQUIRE(pltPlot != nullptr);
}

constexpr int key_delay = 5;

void saveTempSBMLFile(MainWindow *w, const UIPointers &ui,
                      ModalWidgetTimer &mwt, QString tempFileName = "tmp.xml") {
  REQUIRE(!mwt.isRunning());
  QFile file(tempFileName);
  file.remove();
  mwt.addUserAction(tempFileName);
  mwt.start();
  ui.listCompartments->setFocus();
  QApplication::setActiveWindow(w);
  QTest::keyClick(w, Qt::Key_S, Qt::ControlModifier, key_delay);
}

void openTempSBMLFile(MainWindow *w, const UIPointers &ui,
                      ModalWidgetTimer &mwt, QString tempFileName = "tmp.xml") {
  REQUIRE(!mwt.isRunning());
  mwt.addUserAction(tempFileName);
  mwt.start();
  ui.listCompartments->setFocus();
  QApplication::setActiveWindow(w);
  QTest::keyClick(w, Qt::Key_O, Qt::ControlModifier, key_delay);
}

void openABtoC(MainWindow *w, const UIPointers &ui, ModalWidgetTimer &mwt) {
  REQUIRE(!mwt.isRunning());
  //  QTest::keyClick(w, Qt::Key_F, Qt::AltModifier, key_delay);
  //  QTest::keyClick(ui.menuFile, Qt::Key_E, Qt::AltModifier, key_delay);
  //  QTest::keyClick(ui.menuOpen_example_SBML_file, Qt::Key_A, Qt::AltModifier,
  //  key_delay);

  // alt+letter doesn't seem to work for opening menus on mac
  // so write SBML file to disk and open it with ctrl+O instead
  std::unique_ptr<libsbml::SBMLDocument> doc(
      libsbml::readSBMLFromString(sbml_test_data::ABtoC().xml));
  libsbml::SBMLWriter().writeSBML(doc.get(), "tmp.xml");
  openTempSBMLFile(w, ui, mwt);
}

void openThreePixelImage(MainWindow *w, const UIPointers &ui,
                         ModalWidgetTimer &mwt) {
  REQUIRE(!mwt.isRunning());
  // to close setImageDimensions dialog that pops up after loading image
  ModalWidgetTimer mwt2;
#ifndef Q_OS_MAC
  mwt2.start();
  QTest::keyClick(w, Qt::Key_I, Qt::AltModifier, key_delay);
  QTest::keyClick(ui.menuImport, Qt::Key_E, Qt::AltModifier, key_delay);
  QTest::keyClick(ui.menuExample_geometry_image, Qt::Key_S, Qt::AltModifier,
                  key_delay);
#else
  // alt+letter doesn't seem to work for opening menus on mac
  // so write iamge to disk and open it with ctrl+I instead
  QImage img(":/geometry/single-pixels-3x1.png");
  img.save("tmp.png");
  mwt.addUserAction("tmp.png", true, &mwt2);
  mwt.start();
  ui.listCompartments->setFocus();
  QApplication::setActiveWindow(w);
  QTest::keyClick(w, Qt::Key_I, Qt::ControlModifier, key_delay);
#endif
}

void REQUIRE_threePixelImageLoaded(const UIPointers &ui) {
  REQUIRE(ui.lblGeometry->getImage().size() == QSize(3, 1));
  REQUIRE(ui.lblGeometry->getImage().pixel(0, 0) == 0xffffffff);
  REQUIRE(ui.lblGeometry->getImage().pixel(1, 0) == 0xffaaaaaa);
  REQUIRE(ui.lblGeometry->getImage().pixel(2, 0) == 0xff525252);
}

// skip tests on osx due to modal dialog box issues
#ifndef Q_OS_MAC
SCENARIO("Shortcut keys", "[gui][mainwindow]") {
  MainWindow w;
  w.show();
  CAPTURE(QTest::qWaitForWindowExposed(&w));
  ModalWidgetTimer mwt;
  UIPointers ui(&w);
  WHEN("user presses F8") {
    THEN("open About dialog box") {
      // this object closes the next modal window to open
      // after capturing the text in mwc.result
      mwt.start();
      QTest::keyClick(&w, Qt::Key_F8);
      QString correctText = "";
      CAPTURE(mwt.getResult());
      REQUIRE(mwt.getResult().left(correctText.size()) == correctText);
    }
  }
  WHEN("user presses F9") {
    THEN("open About Qt dialog box") {
      mwt.start();
      QTest::keyClick(&w, Qt::Key_F9);
      QString correctText = "<h3>About Qt</h3>";
      CAPTURE(mwt.getResult());
      REQUIRE(mwt.getResult().left(correctText.size()) == correctText);
    }
  }
  WHEN("user presses ctrl+o") {
    THEN("open AcceptOpen FileDialog") {
      mwt.addUserAction(QStringList{"Escape"});
      mwt.start();
      QTest::keyClick(&w, Qt::Key_O, Qt::ControlModifier);
      REQUIRE(mwt.getResult() == "QFileDialog::AcceptOpen");
    }
  }
  WHEN("user presses ctrl+s") {
    THEN("open AcceptSave FileDialog") {
      // open very-simple-model to have something to save
      QTest::keyClick(&w, Qt::Key_F, Qt::AltModifier, key_delay);
      QTest::keyClick(ui.menuFile, Qt::Key_E, Qt::NoModifier, key_delay);
      QTest::keyClick(ui.menuOpen_example_SBML_file, Qt::Key_V, Qt::NoModifier,
                      key_delay);
      mwt.addUserAction(QStringList{"Escape"});
      mwt.start();
      QTest::keyClick(&w, Qt::Key_S, Qt::ControlModifier);
      REQUIRE(mwt.getResult() == "QFileDialog::AcceptSave");
    }
  }
  WHEN("user presses ctrl+tab (no SBML & compartment image loaded)") {
    THEN("remain on Geometry tab: all others disabled") {
      REQUIRE(ui.tabMain->currentIndex() == 0);
      QTest::keyPress(w.windowHandle(), Qt::Key_Tab, Qt::ControlModifier,
                      key_delay);
      REQUIRE(ui.tabMain->currentIndex() == 0);
    }
  }
  WHEN("user presses ctrl+shift+tab (no SBML & compartment image loaded)") {
    THEN("remain on Geometry tab: all others disabled") {
      REQUIRE(ui.tabMain->currentIndex() == 0);
      QTest::keyPress(w.windowHandle(), Qt::Key_Tab,
                      Qt::ControlModifier | Qt::ShiftModifier, key_delay);
      REQUIRE(ui.tabMain->currentIndex() == 0);
    }
  }
}

SCENARIO("click on btnChangeCompartment",
         "[gui][mainwindow][btnChangeCompartment]") {
  MainWindow w;
  w.show();
  CAPTURE(QTest::qWaitForWindowExposed(&w));
  UIPointers ui(&w);
  ModalWidgetTimer mwt1;
  ModalWidgetTimer mwt2;
  WHEN("no valid SBML model loaded") {
    THEN("tell user, offer to load one, user clicks no") {
      mwt1.addUserAction(QStringList{"Escape"});
      mwt1.start();
      QTest::mouseClick(ui.btnChangeCompartment, Qt::LeftButton,
                        Qt::KeyboardModifiers(), QPoint(), key_delay);
      QString correctText = "No valid SBML model loaded - import one now?";
      REQUIRE(mwt1.getResult().left(correctText.size()) == correctText);
    }
    WHEN("user clicks yes") {
      THEN("open SBML import file dialog") {
        // start timer to press spacebar (i.e. accept default "yes") on first
        // modal widget, then close second widget
        mwt1.addUserAction(" ", true, &mwt2);
        mwt2.addUserAction(QStringList{"Escape"});
        mwt1.start();
        // start timer to close second widget (after first timer is done)
        QTest::mouseClick(ui.btnChangeCompartment, Qt::LeftButton,
                          Qt::KeyboardModifiers(), QPoint(), key_delay);
        // check that second widget was a file open dialog
        REQUIRE(mwt2.getResult() == "QFileDialog::AcceptOpen");
      }
    }
  }
  WHEN("valid SBML model loaded, but no geometry image loaded") {
    openABtoC(&w, ui, mwt1);
    THEN("tell user, offer to load one, user clicks no") {
      mwt1.addUserAction(QStringList{"Escape"});
      mwt1.start();
      QTest::mouseClick(ui.btnChangeCompartment, Qt::LeftButton,
                        Qt::KeyboardModifiers(), QPoint(), key_delay);
      QString correctText =
          "No image of compartment geometry loaded - import one now?";
      REQUIRE(mwt1.getResult().left(correctText.size()).toStdString() ==
              correctText.toStdString());
    }
    WHEN("user clicks yes") {
      THEN("open import geometry from image dialog") {
        // start timer to press spacebar (i.e. accept default "yes") on first
        // modal widget, then close second widget
        mwt1.addUserAction(" ", true, &mwt2);
        mwt2.addUserAction(QStringList{"Escape"});
        mwt1.start();
        QTest::mouseClick(ui.btnChangeCompartment, Qt::LeftButton,
                          Qt::KeyboardModifiers(), QPoint(), key_delay);
        // check that second widget was a file open dialog
        REQUIRE(mwt2.getResult() == "QFileDialog::AcceptOpen");
      }
    }
  }
}

SCENARIO("import built-in SBML model and compartment geometry image",
         "[gui][mainwindow]") {
  MainWindow w;
  w.show();
  UIPointers ui(&w);
  ModalWidgetTimer mwt;
  CAPTURE(QTest::qWaitForWindowExposed(&w));
  WHEN("user opens ABtoC model") { openABtoC(&w, ui, mwt); }
  WHEN("user opens ABtoC model, then three-pixel image, then saves SBML") {
    openABtoC(&w, ui, mwt);
    openThreePixelImage(&w, ui, mwt);
    REQUIRE_threePixelImageLoaded(ui);
    saveTempSBMLFile(&w, ui, mwt);
    THEN("user opens saved SBML file: finds image") {
      openTempSBMLFile(&w, ui, mwt);
      REQUIRE_threePixelImageLoaded(ui);
    }
  }
}

SCENARIO("load built-in SBML model very-simple-model", "[gui][mainwindow]") {
  MainWindow w;
  w.show();
  UIPointers ui(&w);
  CAPTURE(QTest::qWaitForWindowExposed(&w));
  REQUIRE(ui.listCompartments->count() == 0);
  // very-simple-model
  QTest::keyClick(&w, Qt::Key_F, Qt::AltModifier, key_delay);
  QTest::keyClick(ui.menuFile, Qt::Key_E, Qt::NoModifier, key_delay);
  QTest::keyClick(ui.menuOpen_example_SBML_file, Qt::Key_V, Qt::NoModifier,
                  key_delay);
  REQUIRE(ui.listCompartments->count() == 3);
  // circadian-clock
  QTest::keyClick(&w, Qt::Key_F, Qt::AltModifier, key_delay);
  QTest::keyClick(ui.menuFile, Qt::Key_E, Qt::NoModifier, key_delay);
  QTest::keyClick(ui.menuOpen_example_SBML_file, Qt::Key_C, Qt::NoModifier,
                  key_delay);
  REQUIRE(ui.listCompartments->count() == 1);
}

SCENARIO("load built-in SBML model, change units", "[gui][mainwindow]") {
  MainWindow w;
  w.show();
  UIPointers ui(&w);
  CAPTURE(QTest::qWaitForWindowExposed(&w));
  REQUIRE(ui.listCompartments->count() == 0);
  // very-simple-model
  QTest::keyClick(&w, Qt::Key_F, Qt::AltModifier, key_delay);
  QTest::keyClick(ui.menuFile, Qt::Key_E, Qt::NoModifier, key_delay);
  QTest::keyClick(ui.menuOpen_example_SBML_file, Qt::Key_V, Qt::NoModifier,
                  key_delay);
  REQUIRE(ui.listCompartments->count() == 3);
  // change units
  ModalWidgetTimer mwt;
  QTest::keyClick(&w, Qt::Key_T, Qt::AltModifier, key_delay);
  mwt.addUserAction({"Down", "Tab", "Down", "Tab", "Down", "Tab", "Down"});
  mwt.start();
  QTest::keyClick(ui.menuTools, Qt::Key_U, Qt::NoModifier, key_delay);
  // save SBML file
  mwt.addUserAction("units.xml");
  mwt.start();
  QTest::keyClick(&w, Qt::Key_S, Qt::ControlModifier, key_delay);
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

SCENARIO("Load SBML file", "[gui][mainwindow]") {
  MainWindow w;
  w.show();
  CAPTURE(QTest::qWaitForWindowExposed(&w));

  UIPointers ui(&w);
  REQUIRE(ui.listCompartments->count() == 0);
  REQUIRE(ui.listMembranes->count() == 0);
  REQUIRE(ui.listSpecies->topLevelItemCount() == 0);
  REQUIRE(ui.listReactions->topLevelItemCount() == 0);
  REQUIRE(ui.listFunctions->count() == 0);

  ModalWidgetTimer mwt;
  std::unique_ptr<libsbml::SBMLDocument> doc(
      libsbml::readSBMLFromString(sbml_test_data::very_simple_model().xml));
  libsbml::SBMLWriter().writeSBML(doc.get(), "tmp.xml");
  openTempSBMLFile(&w, ui, mwt);
  REQUIRE(ui.tabMain->currentIndex() == 0);
  REQUIRE(ui.listCompartments->count() == 3);

  // create geometry image
  QImage img(":/geometry/filled-donut-100x100.png");
  img.save("tmp.png");
  QRgb col1 = img.pixel(0, 0);
  QRgb col2 = img.pixel(40, 30);
  QRgb col3 = img.pixel(50, 50);
  CAPTURE(col1);
  CAPTURE(col2);
  CAPTURE(col3);

  // import Geometry from image
  // mwt to close setImageDimensions dialog that pops up after loading image
  ModalWidgetTimer mwtClose;
  mwt.addUserAction("tmp.png", true, &mwtClose);
  mwt.start();
  ui.listCompartments->setFocus();
  QApplication::setActiveWindow(&w);
  QTest::keyClick(&w, Qt::Key_I, Qt::ControlModifier, key_delay);
  CAPTURE(QTest::qWaitFor(
      [&ui]() { return ui.lblGeometry->getImage().size() != QSize(0, 0); },
      30000));
  REQUIRE(ui.lblGeometry->getImage().size() == img.size());

  // assign geometry
  // c1 -> col1
  ui.listCompartments->setCurrentRow(0);
  QApplication::processEvents();
  ui.btnChangeCompartment->click();
  QApplication::processEvents();
  // click on top-left
  QTest::mouseClick(ui.lblGeometry, Qt::LeftButton, Qt::KeyboardModifiers(),
                    QPoint(1, 1));
  QApplication::processEvents();
  REQUIRE(ui.lblGeometry->getColour() == col1);
  // c2 -> col2
  ui.listCompartments->setCurrentRow(1);
  QApplication::processEvents();
  ui.btnChangeCompartment->click();
  QApplication::processEvents();
  // click on ~(40,30)
  int imgDisplaySize =
      std::min(ui.lblGeometry->width(), ui.lblGeometry->height());
  QTest::mouseClick(ui.lblGeometry, Qt::LeftButton, Qt::KeyboardModifiers(),
                    QPoint(2 * imgDisplaySize / 5, 3 * imgDisplaySize / 10));
  QApplication::processEvents();
  REQUIRE(ui.lblGeometry->getColour() == col2);
  // c3 -> col3
  ui.listCompartments->setCurrentRow(2);
  QApplication::processEvents();
  ui.btnChangeCompartment->click();
  QApplication::processEvents();
  // click on middle
  int imgDisplayWidth =
      std::min(ui.lblGeometry->width(), ui.lblGeometry->height());
  QTest::mouseClick(ui.lblGeometry, Qt::LeftButton, Qt::KeyboardModifiers(),
                    QPoint(imgDisplayWidth / 2, imgDisplayWidth / 2));
  QApplication::processEvents();
  REQUIRE(ui.lblGeometry->getColour() == col3);
  ui.tabCompartmentGeometry->setFocus();
  QApplication::processEvents();
  // display Boundaries sub-tab
  QTest::keyPress(w.windowHandle(), Qt::Key_Tab, Qt::ControlModifier,
                  key_delay);
  QApplication::processEvents();
  QTest::mouseClick(ui.lblCompBoundary, Qt::LeftButton, Qt::KeyboardModifiers(),
                    QPoint(ui.lblCompBoundary->width() / 2,
                           ui.lblCompBoundary->height() / 2));
  QApplication::processEvents();

  QApplication::processEvents();
  // display Mesh sub-tab
  QTest::keyPress(w.windowHandle(), Qt::Key_Tab, Qt::ControlModifier,
                  key_delay);
  QApplication::processEvents();
  REQUIRE(ui.listCompartments->currentRow() == 2);
  QTest::mouseMove(
      ui.lblCompMesh,
      QPoint(ui.lblCompMesh->width() / 3, ui.lblCompMesh->height() / 3),
      mouseDelay);
  QTest::mouseClick(
      ui.lblCompMesh, Qt::LeftButton, Qt::KeyboardModifiers(),
      QPoint(ui.lblCompMesh->width() / 3, ui.lblCompMesh->height() / 3));
  QApplication::processEvents();
  REQUIRE(ui.listCompartments->currentRow() == 1);
  QTest::mouseMove(ui.lblCompMesh, QPoint(6, 6), mouseDelay);
  QTest::mouseClick(ui.lblCompMesh, Qt::LeftButton, Qt::KeyboardModifiers(),
                    QPoint(6, 6));
  QApplication::processEvents();
  REQUIRE(ui.listCompartments->currentRow() == 0);

  // if lblGeometry has focus then ctrl+tab doesn't work to change tabs:
  ui.listCompartments->setCurrentRow(0);
  QApplication::processEvents();
  ui.listCompartments->setFocus();
  QApplication::processEvents();
  // display membrane tab
  QTest::keyPress(w.windowHandle(), Qt::Key_Tab, Qt::ControlModifier,
                  key_delay);
  CAPTURE(QTest::qWaitFor([&ui]() { return ui.tabMain->currentIndex() == 1; }));
  REQUIRE(ui.tabMain->currentIndex() == 1);
  REQUIRE(ui.listMembranes->count() == 2);
  // select each item in listMembranes
  ui.listMembranes->setFocus();
  if (ui.listMembranes->count() > 0) {
    ui.listMembranes->setCurrentRow(0);
  }
  for (int i = 0; i < ui.listMembranes->count(); ++i) {
    QTest::keyClick(ui.listMembranes, Qt::Key_Down, Qt::ControlModifier,
                    key_delay);
    QTest::keyClick(ui.listMembranes, Qt::Key_Enter, Qt::ControlModifier,
                    key_delay);
  }

  // display species tab
  QTest::keyPress(w.windowHandle(), Qt::Key_Tab, Qt::ControlModifier,
                  key_delay);
  QApplication::processEvents();
  REQUIRE(ui.tabMain->currentIndex() == 2);
  REQUIRE(ui.listSpecies->topLevelItemCount() == 3);
  // select first item in listSpecies
  ui.listSpecies->setFocus();
  // ui.listSpecies->setCurrentItem(ui.listSpecies->topLevelItem(0));
  // QTest::keyClick(ui.listSpecies, Qt::Key_Down, Qt::ControlModifier,
  // key_delay);
  REQUIRE(ui.chkSpeciesIsConstant->isChecked() == true);
  REQUIRE(ui.chkSpeciesIsSpatial->isChecked() == true);
  // toggle is spatial checkbox
  QTest::mouseClick(ui.chkSpeciesIsSpatial, Qt::LeftButton,
                    Qt::KeyboardModifiers(), QPoint(1, 1), key_delay);
  REQUIRE(ui.chkSpeciesIsSpatial->isChecked() == false);
  REQUIRE(ui.chkSpeciesIsConstant->isChecked() == true);
  QTest::mouseClick(ui.chkSpeciesIsSpatial, Qt::LeftButton,
                    Qt::KeyboardModifiers(), QPoint(1, 1), key_delay);
  REQUIRE(ui.chkSpeciesIsSpatial->isChecked() == true);
  REQUIRE(ui.chkSpeciesIsConstant->isChecked() == false);
  // toggle is constant checkbox
  QTest::mouseClick(ui.chkSpeciesIsConstant, Qt::LeftButton,
                    Qt::KeyboardModifiers(), QPoint(1, 1), key_delay);
  REQUIRE(ui.chkSpeciesIsConstant->isChecked() == true);
  REQUIRE(ui.chkSpeciesIsSpatial->isChecked() == false);
  QTest::mouseClick(ui.chkSpeciesIsConstant, Qt::LeftButton,
                    Qt::KeyboardModifiers(), QPoint(1, 1), key_delay);
  REQUIRE(ui.chkSpeciesIsConstant->isChecked() == false);
  REQUIRE(ui.chkSpeciesIsSpatial->isChecked() == false);
  // select second item in listSpecies
  QTest::keyClick(ui.listSpecies, Qt::Key_Down, Qt::ControlModifier, key_delay);
  REQUIRE(ui.chkSpeciesIsConstant->isChecked() == false);
  REQUIRE(ui.chkSpeciesIsSpatial->isChecked() == true);
  // toggle is spatial checkbox
  QTest::mouseClick(ui.chkSpeciesIsSpatial, Qt::LeftButton,
                    Qt::KeyboardModifiers(), QPoint(1, 1), key_delay);
  REQUIRE(ui.chkSpeciesIsSpatial->isChecked() == false);
  REQUIRE(ui.chkSpeciesIsConstant->isChecked() == false);
  QTest::mouseClick(ui.chkSpeciesIsSpatial, Qt::LeftButton,
                    Qt::KeyboardModifiers(), QPoint(1, 1), key_delay);
  REQUIRE(ui.chkSpeciesIsSpatial->isChecked() == true);
  QTest::keyClick(ui.listSpecies, Qt::Key_Enter, Qt::ControlModifier,
                  key_delay);
  // keep pressing down until we have selected the B_c3 species
  // (4th in the SBML doc hence has colour index 3)
  for (int i = 0; i < 6; ++i) {
    QTest::keyClick(ui.listSpecies, Qt::Key_Down, Qt::ControlModifier,
                    key_delay);
  }
  REQUIRE(ui.lblSpeciesColour->pixmap()->toImage().pixelColor(0, 0) ==
          utils::indexedColours()[3]);
  // just click Enter, so accept default colour, ie no-op
  mwt.start();
  QTest::mouseClick(ui.btnChangeSpeciesColour, Qt::LeftButton,
                    Qt::KeyboardModifier(), QPoint(), key_delay);
  REQUIRE(ui.lblSpeciesColour->pixmap()->toImage().pixelColor(0, 0) ==
          utils::indexedColours()[3]);

  // display reactions tab
  QTest::keyPress(w.windowHandle(), Qt::Key_Tab, Qt::ControlModifier,
                  key_delay);
  QApplication::processEvents();
  REQUIRE(ui.tabMain->currentIndex() == 3);
  REQUIRE(ui.listReactions->topLevelItemCount() == 5);
  // select each item in listReactions
  ui.listReactions->setFocus();
  ui.listReactions->setCurrentItem(ui.listReactions->topLevelItem(0));
  for (int i = 0; i < 10; ++i) {
    QTest::keyClick(ui.listReactions, Qt::Key_Down, Qt::ControlModifier,
                    key_delay);
    QTest::keyClick(ui.listReactions, Qt::Key_Enter, Qt::ControlModifier,
                    key_delay);
  }

  // display functions tab
  QTest::keyPress(w.windowHandle(), Qt::Key_Tab, Qt::ControlModifier,
                  key_delay);
  QApplication::processEvents();
  REQUIRE(ui.tabMain->currentIndex() == 4);
  REQUIRE(ui.listFunctions->count() == 0);
  // select each item in listSpecies
  ui.listFunctions->setFocus();
  if (ui.listFunctions->count() > 0) {
    ui.listFunctions->setCurrentRow(0);
  }
  for (int i = 0; i < ui.listFunctions->count(); ++i) {
    QTest::keyClick(ui.listFunctions, Qt::Key_Down, Qt::ControlModifier,
                    key_delay);
    QTest::keyClick(ui.listFunctions, Qt::Key_Enter, Qt::ControlModifier,
                    key_delay);
  }

  // display simulate tab
  QApplication::processEvents();
  QTest::keyPress(w.windowHandle(), Qt::Key_Tab, Qt::ControlModifier,
                  key_delay);
  REQUIRE(ui.tabMain->currentIndex() == 5);
  ui.txtSimLength->setText("1000.0");
  ui.txtSimInterval->setText("0.01");
  ui.txtSimDt->setText("0.01");
  // stop simulation after 1 second
  QTimer::singleShot(1000, [=]() {
    QTest::keyClick(ui.pltPlot, Qt::Key_C, Qt::ControlModifier, 0);
  });
  // start simulation
  QTest::mouseClick(ui.btnSimulate, Qt::LeftButton);
  // click on graph
  QTest::mouseClick(ui.pltPlot, Qt::LeftButton);

  // reset simulation
  QTest::mouseClick(ui.btnResetSimulation, Qt::LeftButton,
                    Qt::KeyboardModifiers(), QPoint(), key_delay);
  // click on graph again
  QTest::mouseClick(ui.pltPlot, Qt::LeftButton);

  // display SBML tab
  QTest::keyPress(w.windowHandle(), Qt::Key_Tab, Qt::ControlModifier,
                  key_delay);
  QApplication::processEvents();
  REQUIRE(ui.tabMain->currentIndex() == 6);

  // display DUNE tab
  QTest::keyPress(w.windowHandle(), Qt::Key_Tab, Qt::ControlModifier,
                  key_delay);
  QApplication::processEvents();
  REQUIRE(ui.tabMain->currentIndex() == 7);

  // display GMSH tab
  QTest::keyPress(w.windowHandle(), Qt::Key_Tab, Qt::ControlModifier,
                  key_delay);
  QApplication::processEvents();
  REQUIRE(ui.tabMain->currentIndex() == 8);
}
#endif
