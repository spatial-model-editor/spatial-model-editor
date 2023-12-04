#include "catch_wrapper.hpp"
#include "dialogeditunit.hpp"
#include "qt_test_utils.hpp"
#include <QFile>
#include <QLabel>
#include <QLineEdit>

using namespace sme::test;

struct DialogEditUnitWidgets {
  explicit DialogEditUnitWidgets(const DialogEditUnit *dialog) {
    GET_DIALOG_WIDGET(QLineEdit, txtName);
    GET_DIALOG_WIDGET(QLineEdit, txtMultiplier);
    GET_DIALOG_WIDGET(QLineEdit, txtScale);
    GET_DIALOG_WIDGET(QLineEdit, txtExponent);
    GET_DIALOG_WIDGET(QLabel, lblBaseUnits);
  }
  QLineEdit *txtName;
  QLineEdit *txtMultiplier;
  QLineEdit *txtScale;
  QLineEdit *txtExponent;
  QLabel *lblBaseUnits;
};

TEST_CASE("DialogEditUnit",
          "[gui/dialogs/editunit][gui/dialogs][gui][editunit][units]") {
  sme::model::Unit u{"h", "second", 0, 1, 3600};
  DialogEditUnit dia(u);
  dia.show();
  DialogEditUnitWidgets widgets(&dia);
  REQUIRE(dia.getUnit().kind == u.kind);
  REQUIRE(dia.getUnit().name == u.name);
  REQUIRE(dia.getUnit().multiplier == dbl_approx(u.multiplier));
  REQUIRE(dia.getUnit().scale == u.scale);
  REQUIRE(dia.getUnit().exponent == u.exponent);
  REQUIRE(widgets.lblBaseUnits->text() == "1 h = 3600 second");
  SECTION("valid unit") {
    sendKeyEvents(widgets.txtName, {"Delete", "Backspace", "H", " ", "!"});
    sendKeyEvents(widgets.txtMultiplier, {"Backspace", "Backspace", "Backspace",
                                          "Backspace", ".", "2"});
    sendKeyEvents(widgets.txtScale, {"Backspace", "Backspace", "1", "0"});
    sendKeyEvents(widgets.txtExponent, {"Backspace", "Backspace", "2"});
    REQUIRE(dia.getUnit().kind == u.kind);
    REQUIRE(dia.getUnit().name == "H !");
    REQUIRE(dia.getUnit().multiplier == dbl_approx(0.2));
    REQUIRE(dia.getUnit().scale == 10);
    REQUIRE(dia.getUnit().exponent == 2);
    REQUIRE(widgets.lblBaseUnits->text() == "1 H ! = (0.2 * 10^(10) second)^2");
  }
  SECTION("invalid multiplier") {
    sendKeyEvents(widgets.txtName, {"Delete", "Backspace", "q"});
    sendKeyEvents(widgets.txtScale, {"1", "0"});
    sendKeyEvents(widgets.txtExponent, {"Backspace", "Backspace", "2"});
    sendKeyEvents(widgets.txtMultiplier, {".", ","});
    REQUIRE(dia.getUnit().name == "q");
    // multiplier ".," invalid: unit retains previous valid multiplier value
    REQUIRE(dia.getUnit().multiplier == dbl_approx(3600));
    REQUIRE(dia.getUnit().scale == 10);
    REQUIRE(dia.getUnit().exponent == 2);
    REQUIRE(widgets.lblBaseUnits->text() ==
            "Invalid value: Multiplier must be a double");
  }
  SECTION("invalid scale") {
    sendKeyEvents(widgets.txtScale, {"q"});
    REQUIRE(widgets.lblBaseUnits->text() ==
            "Invalid value: Scale must be an integer");
  }
  SECTION("invalid exponent") {
    sendKeyEvents(widgets.txtExponent, {"q"});
    REQUIRE(widgets.lblBaseUnits->text() ==
            "Invalid value: Exponent must be an integer");
  }
}
