#include "catch_wrapper.hpp"
#include "dialogunits.hpp"
#include "model_test_utils.hpp"
#include "qt_test_utils.hpp"
#include "sme/model.hpp"

using namespace sme::test;

TEST_CASE("DialogUnits", "[gui/dialogs/units][gui/dialogs][gui][units]") {
  auto m{getExampleModel(Mod::ABtoC)};
  SECTION("no default units") {
    auto units = m.getUnits();
    units.setTimeIndex(0);
    units.setLengthIndex(0);
    units.setVolumeIndex(0);
    units.setAmountIndex(0);
    DialogUnits dia(units);
    REQUIRE(dia.getTimeUnitIndex() == 0);
    REQUIRE(dia.getLengthUnitIndex() == 0);
    REQUIRE(dia.getVolumeUnitIndex() == 0);
    REQUIRE(dia.getAmountUnitIndex() == 0);
    ModalWidgetTimer mwt;
    SECTION("user presses down key on each") {
      mwt.addUserAction({"Down", "Tab", "Down", "Tab", "Down", "Tab", "Down"});
      mwt.start();
      dia.exec();
      REQUIRE(dia.getTimeUnitIndex() == 1);
      REQUIRE(dia.getLengthUnitIndex() == 1);
      REQUIRE(dia.getVolumeUnitIndex() == 1);
      REQUIRE(dia.getAmountUnitIndex() == 1);
    }
  }
  SECTION("default units") {
    auto units = m.getUnits();
    units.setTimeIndex(0);
    units.setLengthIndex(3);
    units.setVolumeIndex(2);
    units.setAmountIndex(1);
    DialogUnits dia(units);
    REQUIRE(dia.getTimeUnitIndex() == 0);
    REQUIRE(dia.getLengthUnitIndex() == 3);
    REQUIRE(dia.getVolumeUnitIndex() == 2);
    REQUIRE(dia.getAmountUnitIndex() == 1);
    ModalWidgetTimer mwt;
    SECTION("user changes selection") {
      mwt.addUserAction({"Down", "Tab", "Up", "Tab", "Up", "Tab", "Down"});
      mwt.start();
      dia.exec();
      REQUIRE(dia.getTimeUnitIndex() == 1);
      REQUIRE(dia.getLengthUnitIndex() == 2);
      REQUIRE(dia.getVolumeUnitIndex() == 1);
      REQUIRE(dia.getAmountUnitIndex() == 2);
    }
    SECTION("user presses up a lot for all") {
      mwt.addUserAction({"Up", "Up", "Tab", "Up", "Up", "Up", "Tab", "Up", "Up",
                         "Tab", "Up", "Up", "Up"});
      mwt.start();
      dia.exec();
      REQUIRE(dia.getTimeUnitIndex() == 0);
      REQUIRE(dia.getLengthUnitIndex() == 0);
      REQUIRE(dia.getVolumeUnitIndex() == 0);
      REQUIRE(dia.getAmountUnitIndex() == 0);
    }
    SECTION("user presses down a lot for all") {
      mwt.addUserAction({"Down", "Down", "Down", "Down", "Down", "Down", "Tab",
                         "Down", "Down", "Down", "Down", "Down", "Down", "Tab",
                         "Down", "Down", "Down", "Down", "Down", "Down", "Tab",
                         "Down", "Down", "Down", "Down", "Down", "Down"});
      mwt.start();
      dia.exec();
      REQUIRE(dia.getTimeUnitIndex() == units.getTimeUnits().size() - 1);
      REQUIRE(dia.getLengthUnitIndex() == units.getLengthUnits().size() - 1);
      REQUIRE(dia.getVolumeUnitIndex() == units.getVolumeUnits().size() - 1);
      REQUIRE(dia.getAmountUnitIndex() == units.getAmountUnits().size() - 1);
    }
  }
}
