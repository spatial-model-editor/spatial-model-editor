#include <QtTest>

#include "catch_wrapper.hpp"
#include "dialogunits.hpp"
#include "qt_test_utils.hpp"
#include "sbml.hpp"

// osx CI tests have issues with key presses & modal dialogs
// for now commenting out this test on osx
#ifndef Q_OS_MACOS
SCENARIO("set model units", "[dialogdimensions][gui]") {
  sbml::SbmlDocWrapper doc;
  QFile f(":/models/ABtoC.xml");
  f.open(QIODevice::ReadOnly);
  doc.importSBMLString(f.readAll().toStdString());
  GIVEN("no default units") {
    auto units = doc.modelUnits;
    units.time.index = 0;
    units.length.index = 0;
    units.volume.index = 0;
    units.amount.index = 0;
    DialogUnits dia(units);
    REQUIRE(dia.getTimeUnitIndex() == 0);
    REQUIRE(dia.getLengthUnitIndex() == 0);
    REQUIRE(dia.getVolumeUnitIndex() == 0);
    REQUIRE(dia.getAmountUnitIndex() == 0);
    ModalWidgetTimer mwt;
    WHEN("user presses down key on each") {
      mwt.setKeySeq({"Down", "Tab", "Down", "Tab", "Down", "Tab", "Down"});
      mwt.start();
      dia.exec();
      REQUIRE(dia.getTimeUnitIndex() == 1);
      REQUIRE(dia.getLengthUnitIndex() == 1);
      REQUIRE(dia.getVolumeUnitIndex() == 1);
      REQUIRE(dia.getAmountUnitIndex() == 1);
    }
  }
  GIVEN("default units") {
    auto units = doc.modelUnits;
    units.time.index = 0;
    units.length.index = 3;
    units.volume.index = 2;
    units.amount.index = 1;
    DialogUnits dia(units);
    REQUIRE(dia.getTimeUnitIndex() == 0);
    REQUIRE(dia.getLengthUnitIndex() == 3);
    REQUIRE(dia.getVolumeUnitIndex() == 2);
    REQUIRE(dia.getAmountUnitIndex() == 1);
    ModalWidgetTimer mwt;
    WHEN("user changes selection") {
      mwt.setKeySeq({"Down", "Tab", "Up", "Tab", "Up", "Tab", "Down"});
      mwt.start();
      dia.exec();
      REQUIRE(dia.getTimeUnitIndex() == 1);
      REQUIRE(dia.getLengthUnitIndex() == 2);
      REQUIRE(dia.getVolumeUnitIndex() == 1);
      REQUIRE(dia.getAmountUnitIndex() == 1);
    }
    WHEN("user presses up a lot for all") {
      mwt.setKeySeq({"Up", "Up", "Tab", "Up", "Up", "Up", "Tab", "Up", "Up",
                     "Tab", "Up", "Up", "Up"});
      mwt.start();
      dia.exec();
      REQUIRE(dia.getTimeUnitIndex() == 0);
      REQUIRE(dia.getLengthUnitIndex() == 0);
      REQUIRE(dia.getVolumeUnitIndex() == 0);
      REQUIRE(dia.getAmountUnitIndex() == 0);
    }
    WHEN("user presses down a lot for all") {
      mwt.setKeySeq({"Down", "Down", "Down", "Down", "Down", "Down", "Tab",
                     "Down", "Down", "Down", "Down", "Down", "Down", "Tab",
                     "Down", "Down", "Down", "Down", "Down", "Down", "Tab",
                     "Down", "Down", "Down", "Down", "Down", "Down"});
      mwt.start();
      dia.exec();
      REQUIRE(dia.getTimeUnitIndex() == units.time.units.size() - 1);
      REQUIRE(dia.getLengthUnitIndex() == units.length.units.size() - 1);
      REQUIRE(dia.getVolumeUnitIndex() == units.volume.units.size() - 1);
      REQUIRE(dia.getAmountUnitIndex() == units.amount.units.size() - 1);
    }
  }
}
#endif
